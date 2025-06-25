# Local Generals.io v6 (Qt) 提交规范

## 引言

本文档概述了 Local Generals.io v6 (Qt) 开发中的提交规范 (commit regulations)。目的是确保项目提交历史 (commit history) 的一致性 (consistency)、清晰性 (clarity) 和可维护性 (intainability)。遵循这些指导方针可以改善协作，并使贡献者 (contributors) 更容易理解项目的进展。

## 提交信息 (commit message) 规范

### 1. **格式 (format)**

每个提交信息应遵循以下结构 (structure)：

```
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

- **type**：描述提交目的 (purpose) 的名词（例如 `feat`、`fix`、`docs`、`style`、`refactor`、`chore`）。
- **scope**：提交影响的代码区域（例如 `ui`、`core`、`networking`）。scope 是可选的，只有在相关时使用。
- **subject**：提交所做更改的简短摘要 (summary)，保持在 72 个字符以内。
- **body**：详细描述所做的更改。解释这些更改为何必要以及是如何实现的。保持 body 内容每行 72 个字符以内。
- **footer**：任何相关的 issue 编号或 breaking change。例如，`Closes #42`、`BREAKING CHANGE: Change API` 等。

### 2. **提交类型**

- `feat`：新增功能（feature 的缩写）。
  示例：
  ```
  feat(ui): add custom skin selection option
  ```

- `fix`：修复 bug。
  示例：
  ```
  fix(networking): resolve connection issue during login
  ```

- `docs`：文档更改。
  示例：
  ```
  docs(readme): update installation instructions
  ```

- `style`：代码样式更改（例如格式化、linting）。
  示例：
  ```
  style(ui): improve button alignment
  ```

- `refactor`：重构代码，但不改变功能。
  示例：
  ```
  refactor(core): simplify game state management logic
  ```

- `chore`：更改构建过程、依赖项或维护任务。
  示例：
  ```
  chore(deps): update Qt framework to v6.3
  ```

- `test`：添加或更新测试。
  示例：
  ```
  test(ui): add tests for player selection screen
  ```

- `ci`：与持续集成 (**c**ontinuous **i**ntegration) 相关的更改。
  示例：
  ```
  ci: add deployment pipeline for production
  ```

### 3. **提交主题 (subject)**

- 始终使用命令式 (imperative) 动词（例如 `add`、`remove`、`fix`、`update`）。
- 保持简洁但描述性强。避免模糊的提交信息，如“Fix stuff”或“Changes made”。
- 除非是专有名词或缩写，否则提交信息应使用小写字母。

### 4. **示例提交信息**

- **正确示例：**
  ```
  feat(core): add multiplayer mode support
  ```
- **错误示例：**
  ```
  Adding multiplayer mode
  ```

## 提交大小与频率

### 1. **提交大小 (commit size)**

- **小而聚焦 (focused)**：每个提交应集中于一个单一的逻辑变更。保持提交大小适中，以便于审查和理解。
- **原子提交**：每个提交应代表一个自包含的更改，可以独立测试或回退。
- **避免大而不聚焦的提交**：避免同时更改多个不相关的项目，因为这会使得理解更改的影响变得困难。

### 2. **提交频率 (commit frequency)**

- **频繁提交**：要经常提交更改，以保持项目历史的更新，并使更改更易于追踪。
- **不要等到大改动后再提交**：不要等到大型功能或 bug 修复结束才提交。将工作拆分为较小的部分，并提交每个部分。这也让其他人能够实时看到进展。
- **避免过多小提交**：虽然鼓励频繁提交，但避免做出微小、无关紧要的提交（例如“修复错别字”或“更改格式”）。这会增加提交历史的杂乱并减慢协作速度。

### 3. **提交实践 (commit practice)**

- **编写描述性提交信息**：如前所述，始终编写有意义且描述性的提交信息，以便其他人理解你所做的更改。
- **合并前进行 rebase**：在将功能分支合并到 `develop` 或 `main` 时，应进行 rebase，确保历史记录干净且线性。
- **Squash 提交**：如果有多个小提交可以逻辑上组合在一起，在合并到主分支前进行 squash。这可以减少提交历史中的噪音。

---

Below is the English version of the document for reference:

# Local Generals.io v6 (Qt) Commit Regulations

## Introduction

This document outlines the commit regulations for the development of Local Generals.io v6 (Qt). The goal is to ensure consistency, clarity, and maintainability in the project’s commit history. By following these guidelines, we can improve collaboration and make it easier for contributors to understand the progress of the project.

## Commit Message Guidelines

### 1. **Format**

Each commit message should follow this structure:

```
<type>(<scope>): <subject>
<BLANK LINE>
<body>
<BLANK LINE>
<footer>
```

- **type**: A noun describing the purpose of the commit (e.g., `feat`, `fix`, `docs`, `style`, `refactor`, `chore`).
- **scope**: The area of the codebase the commit affects (e.g., `ui`, `core`, `networking`). The scope is optional and should only be used when relevant.
- **subject**: A short summary of what the commit does. Keep it under 72 characters.
- **body**: A detailed description of the changes made. Explain why the changes were necessary and how they were implemented. Keep the body wrapped at 72 characters per line.
- **footer**: Any relevant issue numbers or breaking changes. For example, `Closes #42`, `BREAKING CHANGE: Change API`, etc.

### 2. **Types of Commit Messages**

#### - `feat`: A new feature.

Example:
```
feat(ui): add custom skin selection option
```

#### - `fix`: A bug fix.

Example:
```
fix(networking): resolve connection issue during login
```

#### - `docs`: Documentation changes.

Example:
```
docs(readme): update installation instructions
```

#### - `style`: Code style changes (e.g., formatting, linting).

Example:
```
style(ui): improve button alignment
```

#### - `refactor`: Refactoring code without changing functionality.

Example:
```
refactor(core): simplify game state management logic
```

#### - `chore`: Changes to the build process, dependencies, or maintenance tasks.

Example:
```
chore(deps): update Qt framework to v6.3
```

#### - `test`: Adding or updating tests.

Example:
```
test(ui): add tests for player selection screen
```

#### - `ci`: Continuous Integration related changes.

Example:
```
ci: add deployment pipeline for production
```

### 3. **Commit Subjects**

- Always start with a verb in the imperative form (e.g., `add`, `remove`, `fix`, `update`).
- Be concise but descriptive. Avoid vague commit messages like "Fix stuff" or "Changes made."
- Use lowercase letters unless the commit message is a proper noun or acronym.

### 4. **Example Commit Messages**

- **Correct Example:**
  ```
  feat(core): add multiplayer mode support
  ```

- **Incorrect Example:**
  ```
  Adding multiplayer mode
  ```

## Commit Size and Frequency

### 1. **Commit Size**

- **Small and Focused**: Each commit should focus on a single logical change. Keep the size manageable so that it’s easy to review and understand. 
- **Atomic Commits**: Each commit should represent a self-contained change that can be tested or reverted independently.
- **Avoid Large, Unfocused Commits**: Avoid making commits that change multiple unrelated areas of the project at once, as this makes it harder to understand the impact of the change.

### 2. **Commit Frequency**

- **Commit Frequently**: Commit your changes often to keep the project history up to date and to make the changes easier to track.
- **Don’t Wait for Large Changes**: Don’t wait until the end of a large feature or bug fix to commit. Break down your work into smaller parts and commit each one. This also allows others to see progress in real time.
- **Avoid Too Many Small Commits**: While frequent commits are encouraged, avoid making tiny, insignificant commits (e.g., "fix typo" or "change formatting"). This can clutter the commit history and slow down collaboration.

### 3. **Commit Practices**

- **Write Descriptive Commit Messages**: As mentioned in the guidelines, always write meaningful and descriptive commit messages so others understand the changes you’ve made.
- **Rebase Before Merging**: Before merging your feature branch into `develop` or `main`, rebase it to ensure a clean and linear history.
- **Squash Commits**: If you have several small commits that can be logically grouped together, squash them before merging into the main branches. This reduces unnecessary noise in the commit history.
