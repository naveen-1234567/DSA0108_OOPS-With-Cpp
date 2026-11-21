# CampusEvents – College Event Management Platform

CSA4302 – Internet Programming (Dept. of CSE)

A production-style college event platform covering the full lifecycle:  
**propose → approve → register → attend (QR) → certify**.

- **Backend**: C++17 + cpp-httplib + SQLite (minimal, focused REST API)
- **Frontend**: React + Vite + Tailwind CSS (primary UI)

---

## 1. Prerequisites

- **g++** 11+ (C++17)
- **CMake** 3.16+
- **Node.js** 18+ and **npm**
- **SQLite3** development package
- **Git**
- Optional: OpenSSL (for stronger hashing; falls back to std::hash if missing)

### Install dependencies

**Ubuntu / Debian**
```bash
sudo apt update
sudo apt install -y g++ cmake libsqlite3-dev libssl-dev nodejs npm git
```

**macOS (Homebrew)**
```bash
brew install cmake sqlite openssl node
```

**Windows**
- Install Visual Studio Build Tools (C++), CMake, Node.js, and SQLite (or use vcpkg / MSYS2).
- Or use WSL2 and follow the Ubuntu steps.

---

## 2. Project layout

```
CampusEvents/   
├── backend/          # C++ REST server
│   ├── include/      # headers (httplib.h, database.h, utils.h)
│   ├── src/          # main.cpp, database.cpp, utils.cpp
│   └── CMakeLists.txt
├── frontend/         # React + Vite app
├── database/         # schema.sql, seed.sql
├── docs/
├── .env.example
└── README.md
```

---

## 3. Setup & Run (exact commands)

### Step 1 – Clone / enter project
```bash
cd CampusEvents
```

### Step 2 – Initialize database
```bash
sqlite3 campus.db < database/schema.sql
sqlite3 campus.db < database/seed.sql
```

### Step 3 – Build backend
```bash
cd backend
cmake -B build
cmake --build build
```

### Step 4 – Run backend (from project root or backend)
```bash
# from CampusEvents/
./backend/build/campus_events_server
# or on Windows: backend\build\campus_events_server.exe
```
Server listens on **http://localhost:8080**

You can set environment variables:
```bash
export PORT=8080
export DB_PATH=campus.db
export CORS_ORIGIN=*
./backend/build/campus_events_server
```

### Step 5 – Run frontend
```bash
cd frontend
npm install
npm run dev
```
Open **http://localhost:5173** in the browser.

The Vite dev server proxies `/api` → `http://localhost:8080`.

---

## 4. Demo accounts (from seed)

| Role         | Email                | Password  |
|--------------|----------------------|-----------|
| SUPER_ADMIN  | admin@college.edu    | admin123  |
| ORGANIZER    | ravi@college.edu     | 123456    |
| ORGANIZER    | anjali@college.edu   | 123456    |
| STUDENT      | aarav@student.edu    | 123456    |
| STUDENT      | sneha@student.edu    | 123456    |

---

## 5. Full walkthrough

1. **Login as Organizer** (`ravi@college.edu` / `123456`)
   - Go to “Propose Event”, fill the form, submit → status becomes **PENDING**.

2. **Login as Super-Admin** (`admin@college.edu` / `admin123`)
   - See the pending queue.
   - Approve the event (or reject / request changes with a reason).

3. **Login as Student** (`aarav@student.edu` / `123456`)
   - Browse approved events → Register.
   - Go to **My Events** → Show QR code.

4. **Login as Organizer again**
   - Open **Scan QR** tab.
   - Use camera or paste the token → mark attendance.
   - Certificate is issued automatically.

5. **Student**
   - Go to **Certificates** → Download.
   - Anyone can verify a code at `/verify/CERT-XXXXXXXXXX`.

---

## 6. API summary

All responses use the envelope:
```json
{ "ok": true, "data": ... }
{ "ok": false, "error": { "code": "...", "message": "..." } }
```

| Method | Path                              | Role          | Description                |
|--------|-----------------------------------|---------------|----------------------------|
| POST   | /api/auth/login                   | public        | Login, returns token       |
| POST   | /api/auth/register                | public        | Register user              |
| GET    | /api/me                           | any           | Current user               |
| GET    | /api/events                       | any           | List (filter status/type)  |
| POST   | /api/events                       | ORGANIZER     | Propose event              |
| PATCH  | /api/events/:id/review            | SUPER_ADMIN   | Approve / reject / changes |
| POST   | /api/events/:id/register          | STUDENT       | Register (seat-limited)    |
| GET    | /api/registrations/mine           | STUDENT       | My registrations + tokens  |
| GET    | /api/registrations/:id/qr         | owner         | QR token                   |
| POST   | /api/attendance/scan              | ORGANIZER     | Scan token → mark attend   |
| GET    | /api/certificates/mine            | STUDENT       | List certificates          |
| GET    | /api/certificates/verify/:code    | public        | Verify certificate         |
| GET    | /api/notifications                | any           | Poll notifications         |

---

## 7. Troubleshooting

| Problem                    | Fix |
|---------------------------|-----|
| CORS errors               | Set `CORS_ORIGIN=*` or the frontend origin; restart server |
| Port 8080 in use          | `PORT=8081 ./backend/build/campus_events_server` |
| `sqlite3` not found       | Install `libsqlite3-dev` / `sqlite` package |
| Camera permission denied  | Allow camera in browser, or use manual token paste |
| Login fails               | Re-seed DB; passwords are SHA-256 of the plain text |
| Frontend proxy fails      | Ensure backend is running on 8080 before `npm run dev` |

---

## 8. Notes on minimality

- Backend is intentionally compact (single binary, layered via Database class).
- Password hashing uses SHA-256 (demo). Replace with bcrypt/argon2 for production.
- JWT is simplified to opaque tokens stored in memory (restart clears sessions).
- Certificates are plain-text files for portability; swap in a PDF library if needed.
- QR is rendered on the frontend with `qrcode.react`; the backend only issues the signed token.

This design keeps the **C++ backend** as the authoritative source of truth for auth, seats, attendance and certificates while the **React frontend** provides the full institutional UI.
