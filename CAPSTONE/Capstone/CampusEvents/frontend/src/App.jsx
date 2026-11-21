import { Routes, Route, Navigate } from 'react-router-dom';
import { useState, useEffect } from 'react';
import Layout from './components/Layout';
import Login from './pages/Login';
import StudentDashboard from './pages/StudentDashboard';
import OrganizerDashboard from './pages/OrganizerDashboard';
import AdminDashboard from './pages/AdminDashboard';
import MyEvents from './pages/MyEvents';
import Certificates from './pages/Certificates';
import VerifyCert from './pages/VerifyCert';
import { ROLES } from './constants';

function App() {
  const [user, setUser] = useState(() => {
    try {
      return JSON.parse(localStorage.getItem('user'));
    } catch {
      return null;
    }
  });

  const login = (userData, token) => {
    localStorage.setItem('token', token);
    localStorage.setItem('user', JSON.stringify(userData));
    setUser(userData);
  };

  const logout = () => {
    localStorage.removeItem('token');
    localStorage.removeItem('user');
    setUser(null);
  };

  if (!user) {
    return (
      <Routes>
        <Route path="/login" element={<Login onLogin={login} />} />
        <Route path="/verify/:code" element={<VerifyCert />} />
        <Route path="*" element={<Navigate to="/login" replace />} />
      </Routes>
    );
  }

  return (
    <Layout user={user} onLogout={logout}>
      <Routes>
        {user.role === ROLES.STUDENT && (
          <>
            <Route path="/" element={<StudentDashboard />} />
            <Route path="/my-events" element={<MyEvents />} />
            <Route path="/certificates" element={<Certificates />} />
          </>
        )}
        {user.role === ROLES.ORGANIZER && (
          <>
            <Route path="/" element={<OrganizerDashboard />} />
            <Route path="/scan" element={<OrganizerDashboard initialTab="scan" />} />
          </>
        )}
        {user.role === ROLES.SUPER_ADMIN && (
          <Route path="/" element={<AdminDashboard />} />
        )}
        <Route path="/verify/:code" element={<VerifyCert />} />
        <Route path="*" element={<Navigate to="/" replace />} />
      </Routes>
    </Layout>
  );
}

export default App;
