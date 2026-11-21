import { useState, useEffect } from 'react';
import { useParams } from 'react-router-dom';
import { certificatesApi } from '../services/api';

export default function VerifyCert() {
  const { code } = useParams();
  const [result, setResult] = useState(null);
  const [error, setError] = useState('');

  useEffect(() => {
    if (!code) return;
    certificatesApi.verify(code)
      .then((r) => {
        if (r.data?.ok) setResult(r.data.data);
        else setError(r.data?.error?.message || 'Invalid');
      })
      .catch((e) => setError(e.response?.data?.error?.message || 'Not found'));
  }, [code]);

  return (
    <div className="min-h-screen flex items-center justify-center bg-gray-50 px-4">
      <div className="bg-white rounded-xl shadow-lg p-8 max-w-md w-full text-center">
        <h1 className="text-xl font-bold text-primary-800 mb-4">Certificate Verification</h1>
        {error && <p className="text-red-600">{error}</p>}
        {result && (
          <div className="space-y-2 text-left">
            <p className="text-green-600 font-semibold text-center">✓ Valid Certificate</p>
            <p><span className="text-gray-500">Student:</span> {result.student_name}</p>
            <p><span className="text-gray-500">Event:</span> {result.event_title}</p>
            <p><span className="text-gray-500">Date:</span> {result.event_date}</p>
            <p><span className="text-gray-500">Code:</span> {result.code}</p>
          </div>
        )}
        {!result && !error && <p className="text-gray-500">Checking…</p>}
      </div>
    </div>
  );
}
