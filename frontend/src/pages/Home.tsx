import { Link } from 'react-router-dom';

export default function Home() {
  return (
    <div className="min-h-screen bg-dark-900 text-gray-100">
      {/* Hero */}
      <header className="border-b border-dark-700">
        <div className="max-w-6xl mx-auto px-8 py-6 flex items-center justify-between">
          <h1 className="text-2xl font-bold text-primary-400">WTLD</h1>
          <div className="flex items-center space-x-4">
            <Link
              to="/login"
              className="px-4 py-2 text-gray-300 hover:text-white transition-colors"
            >
              Войти
            </Link>
            <Link
              to="/register"
              className="px-6 py-2 bg-primary-600 hover:bg-primary-700 text-white font-medium rounded-lg transition-colors"
            >
              Регистрация
            </Link>
          </div>
        </div>
      </header>

      <main>
        {/* Hero Section */}
        <section className="max-w-6xl mx-auto px-8 py-24 text-center">
          <div className="mb-8">
            <span className="text-7xl mb-6 block">📊</span>
            <h2 className="text-5xl font-bold text-white mb-4">
              WTLD — Платформа анализа логов
            </h2>
            <p className="text-xl text-gray-400 max-w-2xl mx-auto">
              Загружайте логи, анализируйте ошибки, обнаруживайте аномалии
              и отслеживайте паттерны — всё в одном месте
            </p>
          </div>

          <div className="flex items-center justify-center space-x-4">
            <Link
              to="/register"
              className="px-8 py-4 bg-primary-600 hover:bg-primary-700 text-white text-lg font-medium rounded-xl transition-colors"
            >
              Начать бесплатно
            </Link>
            <Link
              to="/login"
              className="px-8 py-4 bg-dark-800 hover:bg-dark-700 text-white text-lg font-medium rounded-xl border border-dark-700 transition-colors"
            >
              Войти в аккаунт
            </Link>
          </div>
        </section>

        {/* Features */}
        <section className="max-w-6xl mx-auto px-8 py-16">
          <h3 className="text-3xl font-bold text-white text-center mb-12">
            Возможности
          </h3>
          <div className="grid grid-cols-1 md:grid-cols-3 gap-8">
            <FeatureCard
              icon="📤"
              title="Загрузка логов"
              description="Поддержка текстовых и JSON-форматов. Автоматический парсинг и анализ при загрузке"
            />
            <FeatureCard
              icon="🔍"
              title="Анализ и аномалии"
              description="Обнаружение аномалий, поиск паттернов, расчёт статистики по уровням логов"
            />
            <FeatureCard
              icon="📈"
              title="Аналитика и правила"
              description="Настраиваемые правила анализа, визуализация данных, распределение по уровню серьёзности"
            />
          </div>
        </section>

        {/* Security */}
        <section className="max-w-6xl mx-auto px-8 py-16">
          <div className="bg-dark-800 rounded-2xl p-12 border border-dark-700 text-center">
            <span className="text-5xl mb-6 block">🔒</span>
            <h3 className="text-2xl font-bold text-white mb-4">
              Безопасность
            </h3>
            <p className="text-gray-400 max-w-xl mx-auto">
              Двухфакторная аутентификация (TOTP), JWT-токены, хеширование паролей Argon2id — ваши данные под надёжной защитой
            </p>
          </div>
        </section>

        {/* CTA */}
        <section className="max-w-6xl mx-auto px-8 py-24 text-center">
          <h3 className="text-3xl font-bold text-white mb-6">
            Готовы начать?
          </h3>
          <p className="text-gray-400 mb-8">
            Создайте аккаунт и загрузите первые логи за пару минут
          </p>
          <Link
            to="/register"
            className="inline-block px-10 py-4 bg-primary-600 hover:bg-primary-700 text-white text-lg font-medium rounded-xl transition-colors"
          >
            Зарегистрироваться
          </Link>
        </section>
      </main>

      {/* Footer */}
      <footer className="border-t border-dark-700 py-8 text-center text-gray-500 text-sm">
        <p>WTLD — Платформа анализа логов © {new Date().getFullYear()}</p>
      </footer>
    </div>
  );
}

function FeatureCard({
  icon,
  title,
  description,
}: {
  icon: string;
  title: string;
  description: string;
}) {
  return (
    <div className="bg-dark-800 rounded-xl p-8 border border-dark-700 hover:border-primary-500/50 transition-colors group">
      <span className="text-4xl mb-4 block">{icon}</span>
      <h4 className="text-xl font-semibold text-white mb-3 group-hover:text-primary-400 transition-colors">
        {title}
      </h4>
      <p className="text-gray-400">{description}</p>
    </div>
  );
}
