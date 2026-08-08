type NavIconName = 'scan' | 'dashboard' | 'settings' | 'maintenance' | 'debug';

const common = {
  fill: 'none',
  stroke: 'currentColor',
  strokeWidth: 1.75,
  strokeLinecap: 'round' as const,
  strokeLinejoin: 'round' as const,
};

export function NavIcon({ name }: { name: NavIconName }) {
  switch (name) {
    case 'scan':
      return (
        <svg className="nav-icon" viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="10" cy="10" r="5" {...common} />
          <path d="M16 8l5-5M21 8V3h-5" {...common} />
        </svg>
      );
    case 'dashboard':
      return (
        <svg className="nav-icon" viewBox="0 0 24 24" aria-hidden="true">
          <rect x="3" y="3" width="8" height="8" rx="1.5" {...common} />
          <rect x="13" y="3" width="8" height="5" rx="1.5" {...common} />
          <rect x="13" y="10" width="8" height="11" rx="1.5" {...common} />
          <rect x="3" y="13" width="8" height="8" rx="1.5" {...common} />
        </svg>
      );
    case 'settings':
      return (
        <svg className="nav-icon" viewBox="0 0 24 24" aria-hidden="true">
          <circle cx="12" cy="12" r="3" {...common} />
          <path
            d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M4.93 19.07l1.41-1.41M17.66 6.34l1.41-1.41"
            {...common}
          />
        </svg>
      );
    case 'maintenance':
      return (
        <svg className="nav-icon" viewBox="0 0 24 24" aria-hidden="true">
          <path d="M14.7 6.3a1 1 0 0 0 0 1.4l1.6 1.6a1 1 0 0 0 1.4 0l3.2-3.2a4 4 0 0 1-5.6 5.6l-8.5 8.5a1 1 0 0 1-1.4 0l-1.4-1.4a1 1 0 0 1 0-1.4l8.5-8.5a4 4 0 0 1 5.6-5.6z" {...common} />
        </svg>
      );
    case 'debug':
      return (
        <svg className="nav-icon" viewBox="0 0 24 24" aria-hidden="true">
          <path d="M8 9l-2 2v2.2a3 3 0 0 0 0 5.6V18l2 2h8l2-2v-1.2a3 3 0 0 0 0-5.6V11l-2-2H8z" {...common} />
          <circle cx="12" cy="12" r="2.5" {...common} />
        </svg>
      );
  }
}
