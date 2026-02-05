import StockDashboard from './components/StockDashboard';

export default function Home() {
  return (
    <main>
      <StockDashboard />
      <a
        href="/dashboard.html"
        className="fixed bottom-8 right-8 bg-[#2962ff] hover:bg-[#0039cb] text-white font-bold py-3 px-6 rounded-full shadow-[0_0_20px_rgba(41,98,255,0.5)] transition-all z-50 flex items-center gap-2 hover:scale-105"
        style={{ textDecoration: 'none' }}
      >
        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="M3 3v18h18" /><path d="M18.7 8l-5.1 5.2-2.8-2.7L7 14.3" /></svg>
        Open Dashboard
      </a>
    </main>
  );
}
