export const ROLES = {
  STUDENT: 'STUDENT',
  ORGANIZER: 'ORGANIZER',
  SUPER_ADMIN: 'SUPER_ADMIN',
};

export const EVENT_STATUS = {
  PENDING: 'PENDING',
  APPROVED: 'APPROVED',
  REJECTED: 'REJECTED',
  CHANGES_REQUESTED: 'CHANGES_REQUESTED',
};

export const STATUS_COLORS = {
  PENDING: 'bg-amber-100 text-amber-800 border-amber-200',
  APPROVED: 'bg-green-100 text-green-800 border-green-200',
  REJECTED: 'bg-red-100 text-red-800 border-red-200',
  CHANGES_REQUESTED: 'bg-purple-100 text-purple-800 border-purple-200',
};

export const EVENT_TYPES = ['Workshop', 'Seminar', 'Competition', 'Cultural', 'Sports', 'Academic'];
