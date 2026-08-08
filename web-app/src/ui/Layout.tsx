import { NavLink } from 'react-router-dom';
import { useApp } from '../context/AppContext';
import { ConnectionStatus } from './components/ConnectionStatus';
import { NavIcon } from './components/NavIcon';

const NAV = [
  { to: '/scan', label: 'Скан', icon: 'scan' as const },
  { to: '/dashboard', label: 'Панель', icon: 'dashboard' as const },
  { to: '/settings', label: 'Настройки', icon: 'settings' as const },
  { to: '/maintenance', label: 'Сервис', icon: 'maintenance' as const },
  { to: '/debug', label: 'Отладка', icon: 'debug' as const },
];

function NavItems({ className }: { className: string }) {
  return (
    <nav className={className} aria-label="Основная навигация">
      {NAV.map((item) => (
        <NavLink key={item.to} to={item.to} className={({ isActive }) => `nav-link${isActive ? ' active' : ''}`}>
          <NavIcon name={item.icon} />
          <span>{item.label}</span>
        </NavLink>
      ))}
    </nav>
  );
}

export function Layout({ children }: { children: React.ReactNode }) {
  const { connectionState } = useApp();

  return (
    <div className="app-shell">
      <aside className="app-sidebar">
        <div className="sidebar-brand">
          <div className="brand-mark" aria-hidden="true">
            BC
          </div>
          <div>
            <strong>BikeComp</strong>
            <span>ПК компаньон</span>
          </div>
        </div>
        <NavItems className="sidebar-nav" />
        <div className="sidebar-meta">
          <span className="meta-line">
            Fake BLE: <strong>{connectionState.useFakeBle ? 'вкл' : 'выкл'}</strong>
          </span>
          <span className="meta-line muted">Chrome · Web Bluetooth</span>
        </div>
      </aside>

      <div className="app-body">
        <header className="app-topbar">
          <ConnectionStatus />
        </header>
        <main className="app-main">{children}</main>
      </div>

      <NavItems className="app-bottom-nav" />
    </div>
  );
}
