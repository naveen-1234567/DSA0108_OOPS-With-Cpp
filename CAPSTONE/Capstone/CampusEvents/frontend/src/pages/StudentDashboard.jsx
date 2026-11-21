import { useState, useEffect } from 'react';
import { eventsApi } from '../services/api';
import { STATUS_COLORS, EVENT_TYPES } from '../constants';

export default function StudentDashboard() {
  const [events, setEvents] = useState([]);
  const [loading, setLoading] = useState(true);
  const [typeFilter, setTypeFilter] = useState('');
  const [msg, setMsg] = useState('');

  const load = () => {
    setLoading(true);
    eventsApi.list({ status: 'APPROVED', type: typeFilter || undefined })
      .then((r) => {
        if (r.data?.ok) setEvents(r.data.data || []);
      })
      .finally(() => setLoading(false));
  };

  useEffect(load, [typeFilter]);

  const register = async (id) => {
    setMsg('');
    try {
      const r = await eventsApi.register(id);
      if (r.data?.ok) {
        setMsg('Registered successfully! Check My Events for QR.');
        load();
      } else {
        setMsg(r.data?.error?.message || 'Failed');
      }
    } catch (e) {
      setMsg(e.response?.data?.error?.message || 'Error');
    }
  };

  return (
    <div>
      <div className="flex flex-wrap items-center justify-between gap-4 mb-6">
        <h1 className="text-2xl font-bold text-gray-800">Browse Events</h1>
        <select
          value={typeFilter}
          onChange={(e) => setTypeFilter(e.target.value)}
          className="border rounded-lg px-3 py-2 text-sm"
        >
          <option value="">All types</option>
          {EVENT_TYPES.map((t) => <option key={t} value={t}>{t}</option>)}
        </select>
      </div>

      {msg && (
        <div className="mb-4 p-3 rounded-lg bg-blue-50 text-blue-800 text-sm">{msg}</div>
      )}

      {loading ? (
        <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
          {[1,2,3].map((i) => <div key={i} className="h-48 bg-gray-200 animate-pulse rounded-xl" />)}
        </div>
      ) : events.length === 0 ? (
        <div className="text-center py-16 text-gray-500">
          <p className="text-lg">No approved events found.</p>
          <p className="text-sm mt-1">Check back later or clear filters.</p>
        </div>
      ) : (
        <div className="grid gap-4 sm:grid-cols-2 lg:grid-cols-3">
          {events.map((ev) => {
            const left = Math.max(0, ev.seat_limit - (ev.registered_count || 0));
            const pct = Math.min(100, ((ev.registered_count || 0) / ev.seat_limit) * 100);
            return (
              <div key={ev.id} className="bg-white rounded-xl shadow-sm border border-gray-100 p-5 flex flex-col">
                <div className="flex items-start justify-between gap-2 mb-2">
                  <h2 className="font-semibold text-gray-900 leading-tight">{ev.title}</h2>
                  <span className="text-xs px-2 py-0.5 rounded-full bg-primary-100 text-primary-800 whitespace-nowrap">
                    {ev.type}
                  </span>
                </div>
                <p className="text-sm text-gray-500 mb-3 line-clamp-2">{ev.description}</p>
                <div className="text-xs text-gray-600 space-y-1 mb-3">
                  <div>📍 {ev.venue}</div>
                  <div>📅 {ev.start_at?.replace('T', ' ').slice(0, 16)}</div>
                </div>
                <div className="mt-auto">
                  <div className="flex justify-between text-xs mb-1">
                    <span>{left} seats left</span>
                    <span>{ev.registered_count || 0}/{ev.seat_limit}</span>
                  </div>
                  <div className="h-1.5 bg-gray-100 rounded-full overflow-hidden mb-3">
                    <div className="h-full bg-primary-500 rounded-full" style={{ width: `${pct}%` }} />
                  </div>
                  <button
                    onClick={() => register(ev.id)}
                    disabled={left === 0}
                    className="w-full py-2 rounded-lg bg-primary-700 hover:bg-primary-800 text-white text-sm font-medium disabled:opacity-50 disabled:cursor-not-allowed"
                  >
                    {left === 0 ? 'Full' : 'Register'}
                  </button>
                </div>
              </div>
            );
          })}
        </div>
      )}
    </div>
  );
}
