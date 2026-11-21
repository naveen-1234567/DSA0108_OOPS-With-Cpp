import { Link, useLocation } from 'react-router-dom';
import { useState, useEffect } from 'react';
import { notificationsApi } from '../services/api';
import { ROLES } from '../constants';

export default function Layout({ user, onLogout, children }) {
  const location = useLocation();
  const [notifications, setNotifications] = useState([]);
  const [showNotif, setShowNotif] = useState(false);

  useEffect(() => {
    const load = () => {
      notificationsApi.list().then((r) => {
        if (r.data?.ok) setNotifications(r.data.data || []);
      }).catch(() => {});
    };
    load();
    const t = setInterval(load, 30000);
    return () => clearInterval(t);
  }, []);

  const unread = notifications.filter((n) => !n.is_read).length;

  const markRead = async (id) => {
    await notificationsApi.markRead(id);
    setNotifications((prev) => prev.map((n) => (n.id === id ? { ...n, is_read: 1 } : n)));
  };

  const nav = [];
  if (user.role === ROLES.STUDENT) {
    nav.push({ to: '/', label: 'Browse Events' }, { to: '/my-events', label: 'My Events' }, { to: '/certificates', label: 'Certificates' });
  } else if (user.role === ROLES.ORGANIZER) {
    nav.push({ to: '/', label: 'My Proposals' }, { to: '/scan', label: 'Scan Attendance' });
  } else {
    nav.push({ to: '/', label: 'Review Queue' });
  }

  return (
    <div className="min-h-screen flex flex-col">
      <header className="bg-primary-800 text-white shadow-md">
        <div className="max-w-6xl mx-auto px-4 py-3 flex items-center justify-between">
          <div className="flex items-center gap-6">
            <Link to="/" className="font-bold text-xl tracking-tight">CampusEvents</Link>
            <nav className="hidden sm:flex gap-4 text-sm">
              {nav.map((item) => (
                <Link
                  key={item.to}
                  to={item.to}
                  className={`px-2 py-1 rounded ${location.pathname === item.to ? 'bg-primary-600' : 'hover:bg-primary-700'}`}
                >
                  {item.label}
                </Link>
              ))}
            </nav>
          </div>
          <div className="flex items-center gap-3">
            <div className="relative">
              <button
                onClick={() => setShowNotif(!showNotif)}
                className="relative p-2 rounded hover:bg-primary-700"
                aria-label="Notifications"
              >
                <svg className="w-5 h-5" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                  <path strokeLinecap="round" strokeLinejoin="round" strokeWidth={2} d="M15 17h5l-1.405-1.405A2.032 2.032 0 0118 14.158V11a6.002 6.002 0 00-4-5.659V5a2 2 0 10-4 0v.341C7.67 6.165 6 8.388 6 11v3.159c0 .538-.214 1.055-.595 1.436L4 17h5m6 0v1a3 3 0 11-6 0v-1m6 0H9" />
                </svg>
                {unread > 0 && (
                  <span className="absolute -top-0.5 -right-0.5 bg-accent-500 text-xs w-4 h-4 rounded-full flex items-center justify-center">
                    {unread}
                  </span>
                )}
              </button>
              {showNotif && (
                <div className="absolute right-0 mt-2 w-80 bg-white text-gray-800 rounded-lg shadow-xl border z-50 max-h-96 overflow-auto">
                  <div className="p-3 border-b font-semibold text-sm">Notifications</div>
                  {notifications.length === 0 ? (
                    <p className="p-4 text-sm text-gray-500">No notifications</p>
                  ) : (
                    notifications.map((n) => (
                      <div
                        key={n.id}
                        className={`p-3 text-sm border-b last:border-0 cursor-pointer hover:bg-gray-50 ${!n.is_read ? 'bg-blue-50' : ''}`}
                        onClick={() => markRead(n.id)}
                      >
                        {n.message}
                        <div className="text-xs text-gray-400 mt-1">{n.created_at}</div>
                      </div>
                    ))
                  )}
                </div>
              )}
            </div>
            <div className="text-sm hidden sm:block">
              <span className="font-medium">{user.name}</span>
              <span className="text-primary-200 ml-1">({user.role})</span>
            </div>
            <button onClick={onLogout} className="text-sm px-3 py-1 rounded bg-primary-700 hover:bg-primary-600">
              Logout
            </button>
          </div>
        </div>
      </header>
      <main className="flex-1 max-w-6xl w-full mx-auto px-4 py-6">{children}</main>
      <footer className="text-center text-xs text-gray-400 py-4">
        CampusEvents · CSA4302 Internet Programming · College Event Management Platform
      </footer>
    </div>
  );
}
