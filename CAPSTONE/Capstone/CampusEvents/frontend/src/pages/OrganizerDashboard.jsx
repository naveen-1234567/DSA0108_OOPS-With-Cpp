import { useState, useEffect } from 'react';
import { eventsApi, attendanceApi } from '../services/api';
import { STATUS_COLORS, EVENT_TYPES } from '../constants';
import { Html5Qrcode } from 'html5-qrcode';

export default function OrganizerDashboard({ initialTab = 'events' }) {
  const [tab, setTab] = useState(initialTab);
  const [events, setEvents] = useState([]);
  const [loading, setLoading] = useState(true);
  const [form, setForm] = useState({
    title: '', type: 'Workshop', theme: '', description: '', venue: '',
    start_at: '', end_at: '', seat_limit: 50,
  });
  const [msg, setMsg] = useState('');
  const [scanResult, setScanResult] = useState('');
  const [manualToken, setManualToken] = useState('');
  const [regs, setRegs] = useState([]);
  const [selectedEvent, setSelectedEvent] = useState(null);

  const load = () => {
    setLoading(true);
    eventsApi.list({ mine: 1 })
      .then((r) => { if (r.data?.ok) setEvents(r.data.data || []); })
      .finally(() => setLoading(false));
  };

  useEffect(load, []);

  const create = async (e) => {
    e.preventDefault();
    setMsg('');
    try {
      const r = await eventsApi.create(form);
      if (r.data?.ok) {
        setMsg('Event proposed successfully (PENDING review).');
        setForm({ title: '', type: 'Workshop', theme: '', description: '', venue: '', start_at: '', end_at: '', seat_limit: 50 });
        load();
      } else setMsg(r.data?.error?.message || 'Failed');
    } catch (err) {
      setMsg(err.response?.data?.error?.message || 'Error');
    }
  };

  const viewRegs = async (id) => {
    setSelectedEvent(id);
    const r = await eventsApi.registrations(id);
    if (r.data?.ok) setRegs(r.data.data || []);
  };

  const startScanner = () => {
    const scanner = new Html5Qrcode('qr-reader');
    scanner.start(
      { facingMode: 'environment' },
      { fps: 10, qrbox: 250 },
      async (decoded) => {
        scanner.stop();
        try {
          const r = await attendanceApi.scan(decoded);
          if (r.data?.ok) setScanResult(`Success: ${r.data.data.student_name} – Cert: ${r.data.data.certificate_code}`);
          else setScanResult(r.data?.error?.message || 'Failed');
        } catch (e) {
          setScanResult(e.response?.data?.error?.message || 'Scan error');
        }
      },
      () => {}
    ).catch(() => setScanResult('Camera permission denied or unavailable'));
  };

  const manualScan = async () => {
    if (!manualToken) return;
    try {
      const r = await attendanceApi.scan(manualToken);
      if (r.data?.ok) setScanResult(`Success: ${r.data.data.student_name}`);
      else setScanResult(r.data?.error?.message || 'Failed');
    } catch (e) {
      setScanResult(e.response?.data?.error?.message || 'Error');
    }
  };

  return (
    <div>
      <div className="flex gap-2 mb-6 border-b">
        {['events', 'create', 'scan'].map((t) => (
          <button
            key={t}
            onClick={() => setTab(t)}
            className={`px-4 py-2 text-sm font-medium border-b-2 -mb-px ${tab === t ? 'border-primary-700 text-primary-700' : 'border-transparent text-gray-500'}`}
          >
            {t === 'events' ? 'My Proposals' : t === 'create' ? 'Propose Event' : 'Scan QR'}
          </button>
        ))}
      </div>

      {msg && <div className="mb-4 p-3 rounded-lg bg-blue-50 text-blue-800 text-sm">{msg}</div>}

      {tab === 'events' && (
        loading ? <div className="h-32 bg-gray-200 animate-pulse rounded-xl" /> : (
          <div className="space-y-3">
            {events.map((ev) => (
              <div key={ev.id} className="bg-white rounded-xl border p-4 shadow-sm">
                <div className="flex flex-wrap items-start justify-between gap-2">
                  <div>
                    <h2 className="font-semibold">{ev.title}</h2>
                    <p className="text-sm text-gray-500">{ev.venue} · {ev.start_at}</p>
                  </div>
                  <span className={`text-xs px-2 py-1 rounded-full border ${STATUS_COLORS[ev.status] || ''}`}>
                    {ev.status}
                  </span>
                </div>
                {ev.review_reason && (
                  <p className="mt-2 text-sm text-gray-600 bg-gray-50 p-2 rounded">Reviewer: {ev.review_reason}</p>
                )}
                <div className="mt-3 flex gap-2">
                  <button onClick={() => viewRegs(ev.id)} className="text-sm text-primary-700 hover:underline">
                    View registrations ({ev.registered_count || 0})
                  </button>
                  {ev.status === 'APPROVED' && (
                    <a href={`/api/events/${ev.id}/attendance.csv`} className="text-sm text-primary-700 hover:underline" target="_blank" rel="noreferrer">
                      Export CSV
                    </a>
                  )}
                </div>
                {selectedEvent === ev.id && regs.length > 0 && (
                  <table className="mt-3 w-full text-sm">
                    <thead><tr className="text-left text-gray-500"><th>Name</th><th>Attended</th></tr></thead>
                    <tbody>
                      {regs.map((r) => (
                        <tr key={r.id} className="border-t">
                          <td className="py-1">{r.student_name}</td>
                          <td>{r.attended ? '✓' : '–'}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                )}
              </div>
            ))}
            {events.length === 0 && <p className="text-gray-500">No proposals yet. Create one!</p>}
          </div>
        )
      )}

      {tab === 'create' && (
        <form onSubmit={create} className="bg-white rounded-xl border p-6 shadow-sm max-w-lg space-y-4">
          <div>
            <label className="block text-sm font-medium mb-1">Title</label>
            <input required className="w-full border rounded-lg px-3 py-2" value={form.title} onChange={(e) => setForm({ ...form, title: e.target.value })} />
          </div>
          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-sm font-medium mb-1">Type</label>
              <select className="w-full border rounded-lg px-3 py-2" value={form.type} onChange={(e) => setForm({ ...form, type: e.target.value })}>
                {EVENT_TYPES.map((t) => <option key={t}>{t}</option>)}
              </select>
            </div>
            <div>
              <label className="block text-sm font-medium mb-1">Seat limit</label>
              <input type="number" min="1" required className="w-full border rounded-lg px-3 py-2" value={form.seat_limit} onChange={(e) => setForm({ ...form, seat_limit: +e.target.value })} />
            </div>
          </div>
          <div>
            <label className="block text-sm font-medium mb-1">Venue</label>
            <input required className="w-full border rounded-lg px-3 py-2" value={form.venue} onChange={(e) => setForm({ ...form, venue: e.target.value })} />
          </div>
          <div className="grid grid-cols-2 gap-3">
            <div>
              <label className="block text-sm font-medium mb-1">Start</label>
              <input type="datetime-local" required className="w-full border rounded-lg px-3 py-2" value={form.start_at} onChange={(e) => setForm({ ...form, start_at: e.target.value.replace('T', ' ') + ':00' })} />
            </div>
            <div>
              <label className="block text-sm font-medium mb-1">End</label>
              <input type="datetime-local" className="w-full border rounded-lg px-3 py-2" value={form.end_at} onChange={(e) => setForm({ ...form, end_at: e.target.value.replace('T', ' ') + ':00' })} />
            </div>
          </div>
          <div>
            <label className="block text-sm font-medium mb-1">Description</label>
            <textarea className="w-full border rounded-lg px-3 py-2" rows={3} value={form.description} onChange={(e) => setForm({ ...form, description: e.target.value })} />
          </div>
          <button type="submit" className="w-full py-2.5 bg-primary-700 text-white rounded-lg font-medium hover:bg-primary-800">
            Submit Proposal
          </button>
        </form>
      )}

      {tab === 'scan' && (
        <div className="space-y-4 max-w-md">
          <div id="qr-reader" className="rounded-xl overflow-hidden border" />
          <button onClick={startScanner} className="w-full py-2 bg-primary-700 text-white rounded-lg">
            Start Camera Scanner
          </button>
          <div className="flex gap-2">
            <input
              className="flex-1 border rounded-lg px-3 py-2 text-sm"
              placeholder="Or paste token manually"
              value={manualToken}
              onChange={(e) => setManualToken(e.target.value)}
            />
            <button onClick={manualScan} className="px-4 py-2 bg-gray-800 text-white rounded-lg text-sm">Scan</button>
          </div>
          {scanResult && <p className="p-3 bg-gray-50 rounded-lg text-sm">{scanResult}</p>}
        </div>
      )}
    </div>
  );
}
