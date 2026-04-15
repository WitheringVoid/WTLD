interface LoadingStateProps {
  message?: string;
  className?: string;
}

export default function LoadingState({ message = 'Загрузка...', className }: LoadingStateProps) {
  return (
    <div className={`flex items-center justify-center h-64 ${className || ''}`}>
      <div className="text-gray-400">{message}</div>
    </div>
  );
}
