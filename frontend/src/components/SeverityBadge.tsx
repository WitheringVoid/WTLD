interface SeverityBadgeProps {
  severity: string;
  withBorder?: boolean;
}

const colors: Record<string, string> = {
  critical: 'bg-red-500/10 text-red-400',
  high: 'bg-orange-500/10 text-orange-400',
  medium: 'bg-yellow-500/10 text-yellow-400',
  low: 'bg-green-500/10 text-green-400',
};

const borderColors: Record<string, string> = {
  critical: 'border-red-500/20',
  high: 'border-orange-500/20',
  medium: 'border-yellow-500/20',
  low: 'border-green-500/20',
};

const labels: Record<string, string> = {
  critical: 'Крит.',
  high: 'Высок.',
  medium: 'Сред.',
  low: 'Низк.',
};

export default function SeverityBadge({ severity, withBorder = false }: SeverityBadgeProps) {
  const badgeClass = withBorder
    ? `${colors[severity] || colors.low} border ${borderColors[severity] || borderColors.low}`
    : colors[severity] || colors.low;

  return (
    <span className={`px-2 py-1 rounded text-xs font-medium ${badgeClass}`}>
      {labels[severity] || labels.low}
    </span>
  );
}
