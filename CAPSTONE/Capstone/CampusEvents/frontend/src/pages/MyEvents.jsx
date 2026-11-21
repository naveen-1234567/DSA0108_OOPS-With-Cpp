import { useState, useEffect } from 'react';
import { registrationsApi } from '../services/api';
import { QRCodeSVG } from 'qrcode.react';

export default function MyEvents() {
  const [regs, setRegs] = useState([]);
  const [loading, setLoading] = useState(true);
  const [selected, setSelected] = useState(null);

  useEffect(() => {
    registrationsApi.mine()
      .then((r) => { if (r.data?.ok) setRegs(r.data.data || []); })
      .finally(() => setLoading(false));
  }, []);

  if (loading) return <div className="animate-pulse h-32 bg-gray-200 rounded-xl" />;

  return (
    <div>
      <h1 className="text-2xl font-bold text-gray-800 mb-6">My Events</h1>
      {regs.length === 0 ? (
        <p className="text-gray-500">You have not registered for any events yet.</p>
      ) : (
        <div className="grid gap-4 sm:grid-cols-2">
          {regs.map((r) => (
            <div key={r.id} className="bg-white rounded-xl border p-5 shadow-sm">
              <h2 className="font-semibold">{r.event_title}</h2>
              <p className="text-sm text-gray-500 mt-1">Registered: {r.created_at}</p>
              <p className="text-sm mt-2">
                Attendance:{' '}
                <span className={r.attended ? 'text-green-600 font-medium' : 'text-amber-600'}>
                  {r.attended ? 'Marked' : 'Pending'}
                </span>
              </p>
              <button
                onClick={() => setSelected(selected === r.id ? null : r.id)}
                className="mt-3 text-sm text-primary-700 font-medium hover:underline"
              >
                {selected === r.id ? 'Hide QR' : 'Show QR Code'}
              </button>
              {selected === r.id && (
                <div className="mt-4 flex flex-col items-center p-4 bg-gray-50 rounded-lg">
                  <QRCodeSVG value={r.token} size={180} level="M" />
                  <p className="text-xs text-gray-400 mt-2 break-all max-w-full text-center">{r.token.slice(0, 24)}…</p>
                </div>
              )}
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
