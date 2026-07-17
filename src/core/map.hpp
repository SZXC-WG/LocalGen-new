// Copyright (C) 2026 SZXC Work Group
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/**
 * @file map.hpp
 *
 * LocalGen Module: core
 *
 * Map I/O
 *
 * Supports: `.lg` (v5); `.lgmp` (v6); and `.json` (official).
 */

#include <QBitArray>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QString>
#include <string>

#include "board.hpp"

struct MapMetadata {
    QString title;
    QString author;
    QDateTime creationDateTime;
    QString description;
};

struct MapDocument {
    MapMetadata metadata;
    Board board;
};

namespace {

constexpr quint32 MAGIC_6 = 0x4C47364D;
constexpr int V5_TYPE_CODES[TILE_TYPE_COUNT] = {3, 0, 2, 4, 1, 5, 6, 7};
constexpr tile_type_e V5_TILE_TYPES[TILE_TYPE_COUNT] = {
    TILE_BLANK, TILE_SWAMP,  TILE_MOUNTAIN, TILE_SPAWN,
    TILE_CITY,  TILE_DESERT, TILE_LOOKOUT,  TILE_OBSERVATORY};

inline intmax_t PMod(intmax_t& x) {
    intmax_t res = x & 63;  // 63 = 0b111111
    x >>= 6;
    return res;
}

constexpr int CHAR_AD = 48;

std::string v5Zip(const Board& board) {
    const int row = board.getHeight(), col = board.getWidth();

    std::string strZip;
    intmax_t k1 = row, k2 = col;
    strZip.push_back(PMod(k1) + CHAR_AD);
    strZip.push_back(PMod(k1) + CHAR_AD);
    strZip.push_back(PMod(k2) + CHAR_AD);
    strZip.push_back(PMod(k2) + CHAR_AD);

    for (int i = 1; i <= row; i++)
        for (int j = 1; j <= col; j++) {
            const Tile& tile = board.tileAt(i, j);
            strZip.push_back(tile.occupier + CHAR_AD);

            const int type = tile.type >= 0 && tile.type < TILE_TYPE_COUNT
                                 ? V5_TYPE_CODES[tile.type]
                                 : 0;

            const char ch = (type << 2) + (tile.lit << 1);
            k1 = tile.type == TILE_SPAWN ? 0 : tile.army;

            const bool negative = k1 < 0;
            if (negative) k1 = -k1;

            strZip.push_back(ch + CHAR_AD + negative);

            for (k2 = 1; k2 <= 8; k2++) strZip.push_back(PMod(k1) + CHAR_AD);
        }
    return strZip;
}

Board v5Unzip(std::string strUnzip) {
    strUnzip.push_back('\0');

    int k = 4, p = 0;
    for (; strUnzip[p] != '\0'; p++) strUnzip[p] -= CHAR_AD;

    const int row = (strUnzip[1] << 6) + strUnzip[0];
    const int col = (strUnzip[3] << 6) + strUnzip[2];

    Board board(row, col);
    for (int i = 1; i <= row; i++)
        for (int j = 1; j <= col; j++) {
            Tile& tile = board.tileAt(i, j);
            tile.occupier = strUnzip[k++];
            if (tile.occupier == 0) tile.occupier = -1;
            bool f = strUnzip[k] & 1;
            strUnzip[k] >>= 1;
            tile.lit = strUnzip[k] & 1;
            strUnzip[k] >>= 1;
            const int type = strUnzip[k++];
            tile.type = type >= 0 && type < TILE_TYPE_COUNT
                            ? V5_TILE_TYPES[type]
                            : TILE_BLANK;

            if (tile.type == TILE_SPAWN)
                tile.army = 0;
            else {
                army_t army = 0;
                for (p = 7; p >= 0; p--) army = (army << 6) + strUnzip[k + p];
                tile.army = f ? -army : army;
            }
            k += 8;
        }

    return board;
}

}  // namespace

inline Board openMap_v5(const QString& filename, QString& errMsg) {
    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        errMsg = "Failed to open map file.";
        return {};
    }
    return v5Unzip(mapFile.readLine().toStdString());
}

inline MapDocument openMap_v6(const QString& filename, QString& errMsg) {
    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::ReadOnly)) {
        errMsg = "Failed to open map file.";
        return {};
    }

    QDataStream ds(&mapFile);
    ds.setVersion(QDataStream::Qt_6_7);
    ds.setByteOrder(QDataStream::BigEndian);

    quint32 magic = 0;
    ds >> magic;
    if (magic != MAGIC_6) {
        errMsg = "Invalid map file: bad magic number.";
        return {};
    }

    QString mapTitle, author, description;
    QDateTime creationDateTime;
    ds >> mapTitle >> author >> creationDateTime >> description;

    quint16 w16, h16;
    QByteArray compressed;
    ds >> w16 >> h16 >> compressed;

    if (ds.status() != QDataStream::Ok) {
        errMsg = "Invalid or corrupted map file format.";
        return {};
    }

    if (!creationDateTime.isValid()) {
        errMsg =
            "Invalid .lgmp metadata: creation datetime is"
            " missing or corrupted.";
        return {};
    }

    const MapMetadata metadata{mapTitle, author, creationDateTime, description};

    // decompress map data
    const int width = w16, height = h16;
    QByteArray raw = qUncompress(compressed);
    if (raw.isEmpty()) {
        errMsg = "Failed to decompress map data. The file is corrupted.";
        return {};
    }

    const int expectedBits = width * height * 19;
    const int expectedBytes = (expectedBits + 7) / 8;
    if (raw.size() != expectedBytes) {
        errMsg = "Decompressed data size mismatch. The file is corrupted.";
        return {};
    }

    QBitArray bits = QBitArray::fromBits(raw.constData(), expectedBits);

    // unpack tile from 19 bits
    auto unpackTile = [](quint32 p) {
        Tile tile;
        tile.type = static_cast<tile_type_e>((p >> 16) & 0x7);
        tile.lit = ((p >> 15) & 0x1);
        int val = (p & 0x7FFF) - 16384;
        if (tile.type == TILE_SPAWN)
            tile.spawnTeam = val;
        else
            tile.army = val;
        return tile;
    };

    Board board(height, width);
    int offset = 0;
    for (int r = 1; r <= height; ++r) {
        for (int c = 1; c <= width; ++c) {
            quint32 packed = 0;
            for (int b = 0; b < 19; ++b, ++offset)
                if (bits.testBit(offset)) packed |= (1u << b);
            board.tileAt(r, c) = unpackTile(packed);
        }
    }

    return MapDocument{metadata, board};
}

inline MapDocument openOfficialMap(const QByteArray& data, QString& errMsg) {
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        errMsg = "This is not a valid JSON file.";
        return {};
    }

    QJsonObject obj = doc.object();
    int width = obj.value(QLatin1StringView("width")).toInt(),
        height = obj.value(QLatin1StringView("height")).toInt();
    QString mapStr = obj.value(QLatin1StringView("map")).toString();
    if (width <= 0 || height <= 0 || mapStr.isEmpty()) {
        errMsg = "Missing or invalid map configuration.";
        return {};
    }

    const QString mapTitle = obj.value(QLatin1StringView("title")).toString();
    const QString author = obj.value(QLatin1StringView("username")).toString();
    const QString description =
        obj.value(QLatin1StringView("description")).toString().trimmed();
    const QString createdAt =
        obj.value(QLatin1StringView("created_at")).toString();

    if (mapTitle.isEmpty() || createdAt.isEmpty()) {
        errMsg =
            "Missing required metadata. Official JSON maps must include "
            "non-empty title and created_at fields.";
        return {};
    }

    QDateTime creationDateTime =
        QDateTime::fromString(createdAt, Qt::ISODateWithMs);
    if (!creationDateTime.isValid())
        creationDateTime = QDateTime::fromString(createdAt, Qt::ISODate);

    if (!creationDateTime.isValid()) {
        errMsg =
            "Invalid created_at metadata. Expected an ISO-8601 datetime "
            "string.";
        return {};
    }
    creationDateTime = creationDateTime.toLocalTime();

    QStringList tileList = mapStr.split(',');
    if (tileList.size() != width * height) {
        errMsg = QString(
                     "Inconsistent map data: number of tiles "
                     "(%1) does not match width*height (%2).")
                     .arg(tileList.size())
                     .arg(width * height);
        return {};
    }

    Board board(height, width);
    for (int r = 0; r < height; ++r) {
        for (int c = 0; c < width; ++c) {
            QString tileCode = tileList[r * width + c].trimmed();
            auto& tile = board.tileAt(r + 1, c + 1);
            if (tileCode.startsWith("L_")) {
                tile.lit = true;
                tileCode = tileCode.mid(2);
            }
            if (tileCode.isEmpty()) {
                tile.type = TILE_BLANK;
                continue;
            }
            bool ok;
            int army = tileCode.toInt(&ok);
            if (ok) {
                tile.type = TILE_CITY;
                tile.army = army;
                continue;
            }
            switch (tileCode.at(0).unicode()) {
                case 'g':
                    tile.type = TILE_SPAWN;
                    tile.spawnTeam = tileCode.length() == 1
                                         ? 0
                                         : tileCode.at(1).unicode() - 'A' + 1;
                    continue;
                case 'm': tile.type = TILE_MOUNTAIN; continue;
                case 'l': tile.type = TILE_LOOKOUT; continue;
                case 'o': tile.type = TILE_OBSERVATORY; continue;
                case 'd': tile.type = TILE_DESERT; continue;
                case 's': tile.type = TILE_SWAMP; continue;
                case 'n':
                    tile.type = TILE_NEUTRAL;
                    tile.army = tileCode.mid(1).toInt(&ok);
                    if (ok) continue;
                default:
                    tile.type = TILE_BLANK;
                    tile.occupier = -2;  // mark as unrecognized tile
            }
        }
    }

    MapMetadata metadata{mapTitle, author, creationDateTime, description};
    return MapDocument{metadata, board};
}

inline MapDocument openOfficialMap(const QString& filename, QString& errMsg) {
    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::ReadOnly)) {
        errMsg = "Failed to open map file.";
        return {};
    }
    return openOfficialMap(mapFile.readAll(), errMsg);
}

inline void saveMap_v5(const QString& filename, const Board& board,
                       QString& errMsg) {
    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        errMsg = "Failed to open map file.";
        return;
    }
    mapFile.write(QString::fromStdString(v5Zip(board)).toUtf8());
}

inline void saveMap_v6(const QString& filename, const MapDocument& doc,
                       QString& errMsg) {
    const Board& board = doc.board;
    const MapMetadata& metadata = doc.metadata;
    const int width = board.getWidth(), height = board.getHeight();

    // 19 bits per tile, thus packed into uint32
    auto packTile = [](const Tile& tile) -> quint32 {
        // [18..16] type (3) | [15] lit | [14..0] army+16384
        int type = tile.type, lit = tile.lit,
            val = tile.type == TILE_SPAWN ? tile.spawnTeam : tile.army;
        return (type << 16) | (lit << 15) | (val + 16384);
    };

    QBitArray bits(height * width * 19);
    int offset = 0;
    for (int r = 1; r <= height; ++r) {
        for (int c = 1; c <= width; ++c) {
            quint32 packed = packTile(board.tileAt(r, c));
            for (int b = 0; b < 19; ++b, ++offset) {
                bits.setBit(offset, (packed >> b) & 1);
            }
        }
    }

    // deflate
    const int byteLen = (bits.size() + 7) / 8;
    const uchar* src = reinterpret_cast<const uchar*>(bits.bits());
    QByteArray compressed = qCompress(src, byteLen, 9);

    QFile mapFile(filename);
    if (!mapFile.open(QIODevice::WriteOnly)) {
        errMsg = "Failed to open map file.";
        return;
    }

    QDataStream ds(&mapFile);
    ds.setVersion(QDataStream::Qt_6_7);
    ds.setByteOrder(QDataStream::BigEndian);

    ds << MAGIC_6 << metadata.title << metadata.author
       << metadata.creationDateTime << metadata.description;
    ds << quint16(width) << quint16(height) << compressed;
}
