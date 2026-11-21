# Architecture

## Three-tier overview

```
┌─────────────────────┐
│   React Frontend    │  (Vite + Tailwind)
│  Student / Organizer│
│     / Admin UI      │
└──────────┬──────────┘
           │ HTTP / JSON  (Bearer token)
┌──────────▼──────────┐
│  C++ REST Backend   │  (cpp-httplib)
│  Auth · Events ·    │
│  Register · Attend  │
│  Certificates       │
└──────────┬──────────┘
           │
┌──────────▼──────────┐
│     SQLite DB       │
│  users, events,     │
│  registrations,     │
│  certificates,      │
│  notifications      │
└─────────────────────┘
```

## Module responsibilities

| Module            | Responsibility |
|-------------------|----------------|
| Auth              | Login/register, token issue, role checks |
| EventService      | Propose, list, state machine PENDING→APPROVED/REJECTED/CHANGES_REQUESTED, venue conflict |
| Registration      | Seat-limited transactional insert, unique (event,user) |
| Attendance        | Token lookup + HMAC verification, single-scan enforcement |
| Certificate       | Issue on attendance, public verify by code |
| Notification      | Persist messages; frontend polls every 30 s |

## QR / Attendance sequence

```mermaid
sequenceDiagram
    participant S as Student
    participant FE as Frontend
    participant BE as C++ Backend
    participant DB as SQLite

    S->>FE: Register for event
    FE->>BE: POST /api/events/:id/register
    BE->>DB: BEGIN; check seats; INSERT registration + token
    BE-->>FE: {token}
    FE-->>S: Show QR (token)

    Note over S,BE: At venue
    S->>FE: Show QR to Organizer
    FE->>BE: POST /api/attendance/scan {token}
    BE->>DB: Lookup token, check not attended
    BE->>DB: UPDATE attended=1; INSERT certificate
    BE-->>FE: success + cert code
    FE-->>S: Certificate available
```

## Future upgrades

- Replace in-memory tokens with real JWT (jwt-cpp)
- WebSocket notifications instead of polling
- Real PDF certificates (libharu / PoDoFo)
- bcrypt/argon2 password hashing
