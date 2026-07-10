const STORAGE_KEY = "localgen-theme";
const root = document.documentElement;
const themeColorMeta = document.querySelector('meta[name="theme-color"]');

function resolvedTheme() {
  const stored = window.localStorage.getItem(STORAGE_KEY);
  if (stored === "light" || stored === "dark") {
    return stored;
  }
  return window.matchMedia("(prefers-color-scheme: dark)").matches ? "dark" : "light";
}

function applyTheme(theme) {
  root.dataset.theme = theme;
  root.style.colorScheme = theme;
  if (themeColorMeta) {
    themeColorMeta.setAttribute("content", theme === "light" ? "#f4f5f0" : "#101412");
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
    window.localStorage.setItem(STORAGE_KEY, nextTheme);
    applyTheme(nextTheme);
  });
}

function initRevealAnimations() {
  const prefersReducedMotion = window.matchMedia("(prefers-reduced-motion: reduce)").matches;
  const revealTargets = document.querySelectorAll(
    ".hero-panel, .panel, .card, .stat-card, .sidebar-card, .release-card, .contributor-card, .plot-card, .page-sidecar, .prose-panel, .hero-visual-card, .feature-marquee, .metric-strip"
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

function initTopicTicker() {
  const ticker = document.querySelector("[data-marquee]");
  if (!ticker) {
    return;
  }

  const clone = ticker.innerHTML;
  ticker.insertAdjacentHTML("beforeend", clone);
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
  };

  toggle.addEventListener("click", () => {
    const isOpen = navigation.classList.toggle("is-open");
    toggle.setAttribute("aria-expanded", String(isOpen));
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

  window.matchMedia("(min-width: 961px)").addEventListener("change", (event) => {
    if (event.matches) {
      closeNavigation();
    }
  });
}

window.addEventListener("DOMContentLoaded", () => {
  applyTheme(resolvedTheme());
  initThemeToggle();
  initNavigation();
  initLanguageMenu();
  initRevealAnimations();
  initTopicTicker();
});
