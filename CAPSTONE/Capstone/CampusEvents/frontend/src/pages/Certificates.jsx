import { useState, useEffect } from 'react';
import { certificatesApi } from '../services/api';

export default function Certificates() {
  const [certs, setCerts] = useState([]);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    certificatesApi.mine()
      .then((r) => { if (r.data?.ok) setCerts(r.data.data || []); })
      .finally(() => setLoading(false));
  }, []);

  const download = async (id) => {
    try {
      const r = await certificatesApi.download(id);
      const url = window.URL.createObjectURL(new Blob([r.data]));
      const a = document.createElement('a');
      a.href = url;
      a.download = `certificate-${id}.txt`;
      a.click();
      window.URL.revokeObjectURL(url);
    } catch {
      alert('Download failed');
    }
  };

  if (loading) return <div className="animate-pulse h-32 bg-gray-200 rounded-xl" />;

  return (
    <div>
      <h1 className="text-2xl font-bold text-gray-800 mb-6">My Certificates</h1>
      {certs.length === 0 ? (
        <p className="text-gray-500">No certificates yet. Attend an event to receive one.</p>
      ) : (
        <div className="space-y-3">
          {certs.map((c) => (
            <div key={c.id} className="bg-white rounded-xl border p-4 flex flex-wrap items-center justify-between gap-3 shadow-sm">
              <div>
                <h2 className="font-semibold">{c.event_title}</h2>
                <p className="text-sm text-gray-500">Code: {c.code} · Issued {c.issued_at}</p>
              </div>
              <button
                onClick={() => download(c.id)}
                className="px-4 py-2 bg-primary-700 text-white text-sm rounded-lg hover:bg-primary-800"
              >
                Download
              </button>
            </div>
          ))}
        </div>
      )}
    </div>
  );
}
