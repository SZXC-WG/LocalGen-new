const STORAGE_KEY = "localgen-theme";
const root = document.documentElement;
const themeColorMeta = document.querySelector('meta[name="theme-color"]');

function resolvedTheme() {
  let stored = null;
  try {
    stored = window.localStorage.getItem(STORAGE_KEY);
  } catch (_) {
    // Storage can be unavailable in privacy-restricted contexts.
  }
  if (stored === "light" || stored === "dark") {
    return stored;
  }
  return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
}

function applyTheme(theme) {
  root.dataset.theme = theme;
  root.style.colorScheme = theme;
  if (themeColorMeta) {
    themeColorMeta.setAttribute("content", theme === "light" ? "#f4f6f5" : "#101318");
  }

  const toggle = document.querySelector("[data-theme-toggle]");
  if (!toggle) {
    return;
  }

  const icon = toggle.querySelector("[data-theme-icon]");
  const text = toggle.querySelector("[data-theme-text]");
  const label = theme === "dark" ? toggle.dataset.labelDark : toggle.dataset.labelLight;
  const iconGlyph = theme === "dark" ? "🌙" : "☀️";
  const toggleLabel = toggle.dataset.toggleLabel || toggle.getAttribute("aria-label") || "Toggle theme";

  toggle.setAttribute("aria-pressed", String(theme === "dark"));
  toggle.setAttribute("aria-label", `${toggleLabel} (${label})`);
  toggle.setAttribute("title", `${toggleLabel} (${label})`);
  if (icon) {
    icon.textContent = iconGlyph;
  }
  if (text) {
    text.textContent = label;
  }
}

function initThemeToggle() {
  const toggle = document.querySelector("[data-theme-toggle]");
  if (!toggle) {
    return;
  }

  toggle.addEventListener("click", () => {
    const nextTheme = root.dataset.theme === "dark" ? "light" : "dark";
    try {
      window.localStorage.setItem(STORAGE_KEY, nextTheme);
    } catch (_) {
      // The theme still applies for the current page view.
    }
    applyTheme(nextTheme);
  });
}

function initRevealAnimations() {
  const prefersReducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const revealTargets = document.querySelectorAll(
    ".game-preview-card, .status-card, .path-card, .route-card, .panel, .card, .stat-card, .sidebar-card, .release-card, .release-index-card, .contributor-card, .docs-card, .page-sidecar, .prose-panel, .metric-strip"
  );

  if (prefersReducedMotion || !("IntersectionObserver" in window)) {
    revealTargets.forEach((element) => element.classList.add("is-visible"));
    return;
  }

  const observer = new IntersectionObserver(
    (entries) => {
      entries.forEach((entry) => {
        if (entry.isIntersecting) {
          entry.target.classList.add("is-visible");
          observer.unobserve(entry.target);
        }
      });
    },
    {
      threshold: 0.16,
      rootMargin: "0px 0px -8% 0px",
    }
  );

  revealTargets.forEach((element, index) => {
    element.classList.add("reveal-item");
    element.style.setProperty("--reveal-delay", `${Math.min(index % 8, 6) * 70}ms`);
    observer.observe(element);
  });
}

function initLanguageMenu() {
  const menu = document.querySelector("[data-language-menu]");
  if (!menu) {
    return;
  }

  document.addEventListener("click", (event) => {
    if (!menu.open) {
      return;
    }

    if (!menu.contains(event.target)) {
      menu.open = false;
    }
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
      menu.open = false;
    }
  });
}

function initNavigation() {
  const toggle = document.querySelector("[data-nav-toggle]");
  const navigation = document.querySelector("[data-site-navigation]");
  if (!toggle || !navigation) {
    return;
  }

  const closeNavigation = () => {
    navigation.classList.remove("is-open");
    toggle.setAttribute("aria-expanded", "false");
    document.body.classList.remove("nav-open");
  };

  toggle.addEventListener("click", () => {
    const isOpen = navigation.classList.toggle("is-open");
    toggle.setAttribute("aria-expanded", String(isOpen));
    document.body.classList.toggle("nav-open", isOpen);
  });

  navigation.addEventListener("click", (event) => {
    if (event.target.closest("a")) {
      closeNavigation();
    }
  });

  document.addEventListener("keydown", (event) => {
    if (event.key === "Escape") {
      closeNavigation();
    }
  });

  document.addEventListener("click", (event) => {
    if (!navigation.classList.contains("is-open")) {
      return;
    }
    if (!navigation.contains(event.target) && !toggle.contains(event.target)) {
      closeNavigation();
    }
  });

  const desktopQuery = window.matchMedia("(min-width: 961px)");
  const handleDesktopChange = (event) => {
    if (event.matches) {
      closeNavigation();
    }
  };
  if (typeof desktopQuery.addEventListener === "function") {
    desktopQuery.addEventListener("change", handleDesktopChange);
  } else if (typeof desktopQuery.addListener === "function") {
    desktopQuery.addListener(handleDesktopChange);
  }
}

function initCodeCopy() {
  const copyLabel = document.body.dataset.copyLabel || "Copy";
  const copiedLabel = document.body.dataset.copiedLabel || "Copied";

  document.querySelectorAll(".prose pre").forEach((block) => {
    const code = block.querySelector("code");
    if (!code || block.querySelector(".code-copy-button")) {
      return;
    }

    const button = document.createElement("button");
    button.type = "button";
    button.className = "code-copy-button";
    button.textContent = copyLabel;
    button.setAttribute("aria-label", copyLabel);
    button.addEventListener("click", async () => {
      try {
        await navigator.clipboard.writeText(code.textContent || "");
        button.textContent = copiedLabel;
        window.setTimeout(() => {
          button.textContent = copyLabel;
        }, 1600);
      } catch (_) {
        button.textContent = copyLabel;
      }
    });
    block.appendChild(button);
  });
}

window.addEventListener("DOMContentLoaded", () => {
  applyTheme(resolvedTheme());
  initThemeToggle();
  initNavigation();
  initLanguageMenu();
  initRevealAnimations();
  initCodeCopy();
});
