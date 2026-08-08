import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { BrowserRouter, Navigate, Route, Routes } from 'react-router-dom';
import { AppProvider } from './context/AppContext';
import { Layout } from './ui/Layout';
import { ScanScreen } from './ui/screens/ScanScreen';
import { DashboardScreen } from './ui/screens/DashboardScreen';
import { SettingsScreen } from './ui/screens/SettingsScreen';
import { MaintenanceScreen } from './ui/screens/MaintenanceScreen';
import { DebugScreen } from './ui/screens/DebugScreen';
import './styles.css';

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <BrowserRouter>
      <AppProvider>
        <Layout>
          <Routes>
            <Route path="/" element={<Navigate to="/scan" replace />} />
            <Route path="/scan" element={<ScanScreen />} />
            <Route path="/dashboard" element={<DashboardScreen />} />
            <Route path="/settings" element={<SettingsScreen />} />
            <Route path="/maintenance" element={<MaintenanceScreen />} />
            <Route path="/debug" element={<DebugScreen />} />
          </Routes>
        </Layout>
      </AppProvider>
    </BrowserRouter>
  </StrictMode>,
);
