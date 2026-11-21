import axios from 'axios';

const api = axios.create({
  baseURL: '/api',
  headers: { 'Content-Type': 'application/json' },
});

api.interceptors.request.use((config) => {
  const token = localStorage.getItem('token');
  if (token) config.headers.Authorization = `Bearer ${token}`;
  return config;
});

api.interceptors.response.use(
  (res) => res,
  (err) => {
    if (err.response?.status === 401) {
      localStorage.removeItem('token');
      localStorage.removeItem('user');
      window.location.href = '/login';
    }
    return Promise.reject(err);
  }
);

export const authApi = {
  login: (email, password) => api.post('/auth/login', { email, password }),
  register: (data) => api.post('/auth/register', data),
  me: () => api.get('/me'),
};

export const eventsApi = {
  list: (params) => api.get('/events', { params }),
  get: (id) => api.get(`/events/${id}`),
  create: (data) => api.post('/events', data),
  review: (id, status, reason) => api.patch(`/events/${id}/review`, { status, reason }),
  register: (id) => api.post(`/events/${id}/register`),
  registrations: (id) => api.get(`/events/${id}/registrations`),
  attendanceCsv: (id) => api.get(`/events/${id}/attendance.csv`, { responseType: 'blob' }),
};

export const registrationsApi = {
  mine: () => api.get('/registrations/mine'),
  qr: (id) => api.get(`/registrations/${id}/qr`),
};

export const attendanceApi = {
  scan: (token) => api.post('/attendance/scan', { token }),
};

export const certificatesApi = {
  mine: () => api.get('/certificates/mine'),
  download: (id) => api.get(`/certificates/${id}/download`, { responseType: 'blob' }),
  verify: (code) => api.get(`/certificates/verify/${code}`),
};

export const notificationsApi = {
  list: () => api.get('/notifications'),
  markRead: (id) => api.patch(`/notifications/${id}/read`),
};

export default api;
