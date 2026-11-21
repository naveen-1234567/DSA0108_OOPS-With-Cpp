-- Seed data for CampusEvents
-- Passwords are SHA256 of the plain password for simplicity (demo only; use bcrypt in production)

INSERT INTO users (name, email, password_hash, role, department) VALUES
('Dr. Priya Sharma', 'admin@college.edu', '240be518fabd2724ddb6f04eeb1da5967448d7e831c08c8fa822809f74c720a9', 'SUPER_ADMIN', 'CSE'),  -- password: admin123
('Prof. Ravi Kumar', 'ravi@college.edu', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'ORGANIZER', 'CSE'), -- password: 123456
('Ms. Anjali Mehta', 'anjali@college.edu', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'ORGANIZER', 'ECE'), -- password: 123456
('Aarav Patel', 'aarav@student.edu', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'STUDENT', 'CSE'), -- password: 123456
('Sneha Reddy', 'sneha@student.edu', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'STUDENT', 'CSE'),
('Vikram Singh', 'vikram@student.edu', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'STUDENT', 'ECE'),
('Meera Iyer', 'meera@student.edu', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'STUDENT', 'MECH'),
('Karan Joshi', 'karan@student.edu', '8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92', 'STUDENT', 'CSE');

INSERT INTO events (title, type, theme, description, venue, start_at, end_at, seat_limit, status, created_by) VALUES
('AI & ML Workshop', 'Workshop', 'Technology', 'Hands-on workshop on machine learning fundamentals and popular frameworks.', 'Seminar Hall A', '2026-11-20 10:00:00', '2026-11-20 16:00:00', 50, 'APPROVED', 2),
('Hackathon 2026', 'Competition', 'Innovation', '24-hour coding competition. Form teams of 3-4 and solve real-world problems.', 'Main Auditorium', '2026-11-25 09:00:00', '2026-11-26 09:00:00', 100, 'APPROVED', 2),
('Cultural Night', 'Cultural', 'Arts', 'Annual cultural evening with dance, music and drama performances.', 'Open Air Theatre', '2026-12-05 18:00:00', '2026-12-05 22:00:00', 200, 'PENDING', 3),
('IoT Seminar', 'Seminar', 'Technology', 'Guest lecture on Internet of Things and smart campus applications.', 'Conference Room 1', '2026-11-18 14:00:00', '2026-11-18 16:00:00', 40, 'APPROVED', 3),
('Sports Meet', 'Sports', 'Fitness', 'Inter-department sports competition covering athletics and team games.', 'Sports Complex', '2026-12-10 08:00:00', '2026-12-10 17:00:00', 150, 'CHANGES_REQUESTED', 2),
('Research Paper Presentation', 'Academic', 'Research', 'Students present their research work. Best paper award included.', 'Lecture Hall 3', '2026-11-22 11:00:00', '2026-11-22 15:00:00', 30, 'REJECTED', 3);

UPDATE events SET review_reason = 'Please add estimated budget and list of judges.' WHERE id = 5;
UPDATE events SET review_reason = 'Overlaps with another department event. Reschedule.' WHERE id = 6;
UPDATE events SET reviewer_id = 1, updated_at = datetime('now') WHERE id IN (5,6);

INSERT INTO notifications (user_id, message) VALUES
(2, 'Your event "Sports Meet" requires changes: Please add estimated budget and list of judges.'),
(3, 'Your event "Research Paper Presentation" was rejected: Overlaps with another department event. Reschedule.'),
(3, 'Your event "Cultural Night" is pending review.');
