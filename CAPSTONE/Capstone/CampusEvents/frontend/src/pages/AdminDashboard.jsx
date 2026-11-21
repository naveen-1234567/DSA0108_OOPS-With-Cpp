import { useState, useEffect } from 'react';
import { eventsApi } from '../services/api';
import { STATUS_COLORS } from '../constants';

export default function AdminDashboard() {
  const [events, setEvents] = useState([]);
  const [loading, setLoading] = useState(true);
  const [reason, setReason] = useState({});
  const [msg, setMsg] = useState('');

  const load = () => {
    setLoading(true);
    eventsApi.list({ status: 'PENDING' })
      .then((r) => { if (r.data?.ok) setEvents(r.data.data || []); })
      .finally(() => setLoading(false));
  };

  useEffect(load, []);

  const review = async (id, status) => {
    const rsn = reason[id] || '';
    if (status !== 'APPROVED' && !rsn.trim()) {
      setMsg('Reason is required for reject / request changes');
      return;
    }
    setMsg('');
    try {
      const r = await eventsApi.review(id, status, rsn);
      if (r.data?.ok) {
        setMsg(`Event ${status}`);
        load();
      } else setMsg(r.data?.error?.message || 'Failed');
    } catch (e) {
      setMsg(e.response?.data?.error?.message || 'Error');
    }
  };

  return (
    <div>
      <h1 className="text-2xl font-bold text-gray-800 mb-2">Review Queue</h1>
      <p className="text-gray-500 text-sm mb-6">Approve, reject or request changes for pending event proposals.</p>

      {msg && <div className="mb-4 p-3 rounded-lg bg-blue-50 text-blue-800 text-sm">{msg}</div>}

      {loading ? (
        <div className="h-32 bg-gray-200 animate-pulse rounded-xl" />
      ) : events.length === 0 ? (
        <p className="text-gray-500">No pending events. All caught up!</p>
      ) : (
        <div className="space-y-4">
          {events.map((ev) => (
            <div key={ev.id} className="bg-white rounded-xl border p-5 shadow-sm">
              <div className="flex flex-wrap justify-between gap-2 mb-2">
                <h2 className="font-semibold text-lg">{ev.title}</h2>
                <span className={`text-xs px-2 py-1 rounded-full border ${STATUS_COLORS.PENDING}`}>PENDING</span>
              </div>
              <p className="text-sm text-gray-600 mb-2">{ev.description}</p>
              <div className="text-xs text-gray-500 space-x-3 mb-3">
                <span>{ev.type}</span>
                <span>📍 {ev.venue}</span>
                <span>📅 {ev.start_at}</span>
                <span>Seats: {ev.seat_limit}</span>
              </div>
              <textarea
                placeholder="Reason (required for reject / changes)"
                className="w-full border rounded-lg px-3 py-2 text-sm mb-3"
                rows={2}
                value={reason[ev.id] || ''}
                onChange={(e) => setReason({ ...reason, [ev.id]: e.target.value })}
              />
              <div className="flex flex-wrap gap-2">
                <button onClick={() => review(ev.id, 'APPROVED')} className="px-4 py-2 bg-green-600 text-white text-sm rounded-lg hover:bg-green-700">
                  Approve
                </button>
                <button onClick={() => review(ev.id, 'CHANGES_REQUESTED')} className="px-4 py-2 bg-purple-600 text-white text-sm rounded-lg hover:bg-purple-700">
                  Request Changes
                </button>
                <button onClick={() => review(ev.id, 'REJECTED')} className="px-4 py-2 bg-red-600 text-white text-sm rounded-lg hover:bg-red-700">
                  Reject
                </button>
              </div>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
