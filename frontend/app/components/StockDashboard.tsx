"use client";
import { useState, useEffect } from 'react';
import { Stock, Summary, Transaction } from '../types';
import { Card, CardHeader } from './ui-card';
import DSAInfo from './DSAInfo';
import CorrelationGraph from './CorrelationGraph';
import { TrendingUp, TrendingDown, RefreshCw, Wallet, LayoutDashboard, PlusCircle, DollarSign } from 'lucide-react';

export default function StockDashboard() {
    const [stocks, setStocks] = useState<Stock[]>([]);
    const [summary, setSummary] = useState<Summary | null>(null);
    const [topGainer, setTopGainer] = useState<Stock | null>(null);
    const [topLoser, setTopLoser] = useState<Stock | null>(null);
    const [loading, setLoading] = useState(true);
    const [showAddModal, setShowAddModal] = useState(false);

    // Form Stats
    const [newName, setNewName] = useState('');
    const [newPrice, setNewPrice] = useState('');
    const [newQty, setNewQty] = useState('');

    const [updateName, setUpdateName] = useState('');
    const [updatePrice, setUpdatePrice] = useState('');
    const [updateQty, setUpdateQty] = useState('');

    const fetchData = async () => {
        setLoading(true);
        try {
            const [stocksRes, summaryRes, topRes] = await Promise.all([
                fetch('http://localhost:5000/api/stocks'),
                fetch('http://localhost:5000/api/summary'),
                fetch('http://localhost:5000/api/top')
            ]);

            const stocksData = await stocksRes.json();
            const summaryData = await summaryRes.json();
            const topData = await topRes.json();

            setStocks(stocksData);
            setSummary(summaryData);
            setTopGainer(topData.topGainer);
            setTopLoser(topData.topLoser);
        } catch (error) {
            console.error("Failed to fetch data", error);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        fetchData();
        const interval = setInterval(fetchData, 5000); // Poll every 5s
        return () => clearInterval(interval);
    }, []);

    const handleAddStock = async (e: React.FormEvent) => {
        e.preventDefault();
        await fetch('http://localhost:5000/api/stocks', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ name: newName.toUpperCase(), buyPrice: Number(newPrice), quantity: Number(newQty) })
        });
        setNewName(''); setNewPrice(''); setNewQty(''); setShowAddModal(false);
        fetchData();
    };

    const handleUpdatePrice = async (e: React.FormEvent) => {
        e.preventDefault();
        await fetch('http://localhost:5000/api/price', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({
                name: updateName.toUpperCase(),
                newPrice: Number(updatePrice),
                newQty: Number(updateQty) // Send quantity
            })
        });
        setUpdateName(''); setUpdatePrice(''); setUpdateQty('');
        fetchData();
    };

    return (
        <div className="min-h-screen p-8 text-white">
            <header className="flex justify-between items-center mb-12">
                <div>
                    <h1 className="text-4xl font-extrabold tracking-in text-white">Dynamic Stock Portfolio <span className="text-[#7f5af0]">Analyzer</span></h1>
                    <p className="text-gray-400 mt-2 text-lg">DSA-Optimized High Performance Tech Stack</p>
                </div>
                <div className="flex gap-4">
                    <button onClick={fetchData} className="p-3 bg-white/5 rounded-full hover:bg-white/10 transition">
                        <RefreshCw className={`w-6 h-6 ${loading ? 'animate-spin' : ''}`} />
                    </button>
                    <button onClick={() => setShowAddModal(!showAddModal)} className="btn-primary flex items-center gap-2 shadow-[0_0_20px_rgba(127,90,240,0.3)]">
                        <PlusCircle className="w-5 h-5" /> Add Stock
                    </button>
                </div>
            </header>

            {/* QUICK STATS */}
            <div className="grid grid-cols-1 md:grid-cols-4 gap-6 mb-8">
                <Card className="relative overflow-hidden group">
                    <div className="absolute top-0 right-0 p-4 opacity-10 group-hover:opacity-20 transition">
                        <Wallet className="w-24 h-24" />
                    </div>
                    <p className="text-gray-400 text-sm font-medium">Total Investment</p>
                    <h3 className="text-3xl font-bold mt-1">₹{summary?.totalInvestment.toFixed(2) || '0.00'}</h3>
                </Card>
                <Card className="relative overflow-hidden group">
                    <div className="absolute top-0 right-0 p-4 opacity-10 group-hover:opacity-20 transition">
                        <DollarSign className="w-24 h-24" />
                    </div>
                    <p className="text-gray-400 text-sm font-medium">Current Value</p>
                    <h3 className="text-3xl font-bold mt-1">₹{summary?.currentValue.toFixed(2) || '0.00'}</h3>
                </Card>
                <Card className="relative overflow-hidden group">
                    <div className="absolute top-0 right-0 p-4 opacity-10 group-hover:opacity-20 transition">
                        <TrendingUp className="w-24 h-24" />
                    </div>
                    <p className="text-gray-400 text-sm font-medium">Total Profit</p>
                    <h3 className={`text-3xl font-bold mt-1 ${summary?.profit && summary.profit >= 0 ? 'text-emerald-400' : 'text-red-400'}`}>
                        {summary?.profit && summary.profit >= 0 ? '+' : ''}{summary?.profit.toFixed(2) || '0.00'}
                    </h3>
                </Card>
                <Card className="relative overflow-hidden bg-gradient-to-br from-[#151425] to-[#7f5af0]/10">
                    <p className="text-gray-400 text-sm font-medium">Active Stocks</p>
                    <h3 className="text-3xl font-bold mt-1">{summary?.stockCount || 0}</h3>
                </Card>
            </div>

            <div className="grid grid-cols-1 lg:grid-cols-3 gap-8 mb-8">
                {/* TOP MOVERS */}
                <div className="lg:col-span-1 space-y-6">
                    <Card className="border-t-4 border-t-emerald-500">
                        <div className="flex items-center gap-3 mb-2">
                            <TrendingUp className="text-emerald-400" />
                            <h4 className="font-bold text-lg">Top Gainer</h4>
                        </div>
                        {topGainer ? (
                            <div>
                                <div className="flex justify-between items-end">
                                    <span className="text-2xl font-bold">{topGainer.name}</span>
                                    <span className="text-emerald-400 font-mono text-xl">+{topGainer.percentGain.toFixed(2)}%</span>
                                </div>
                                <p className="text-gray-500 text-sm mt-1">₹{topGainer.currentPrice}</p>
                            </div>
                        ) : <p className="text-gray-500 italic">No data</p>}
                    </Card>

                    <Card className="border-t-4 border-t-red-500">
                        <div className="flex items-center gap-3 mb-2">
                            <TrendingDown className="text-red-400" />
                            <h4 className="font-bold text-lg">Top Loser</h4>
                        </div>
                        {topLoser ? (
                            <div>
                                <div className="flex justify-between items-end">
                                    <span className="text-2xl font-bold">{topLoser.name}</span>
                                    <span className="text-red-400 font-mono text-xl">{topLoser.percentGain.toFixed(2)}%</span>
                                </div>
                                <p className="text-gray-500 text-sm mt-1">₹{topLoser.currentPrice}</p>
                            </div>
                        ) : <p className="text-gray-500 italic">No data</p>}
                    </Card>

                    {/* UPDATE FORM */}
                    <Card>
                        <CardHeader title="Quick Update" subtitle="Update stock price efficiently" />
                        <form onSubmit={handleUpdatePrice} className="space-y-3">
                            <input className="input-field" placeholder="Stock Symbol" value={updateName} onChange={e => setUpdateName(e.target.value)} required />
                            <div className="grid grid-cols-2 gap-2">
                                <input className="input-field" type="number" step="0.01" placeholder="New Price" value={updatePrice} onChange={e => setUpdatePrice(e.target.value)} required />
                                <input className="input-field" type="number" placeholder="New Qty" value={updateQty} onChange={e => setUpdateQty(e.target.value)} />
                            </div>
                            <button type="submit" className="w-full btn-primary bg-white/10 hover:bg-white/20 text-white">Update</button>
                        </form>
                    </Card>
                </div>

                {/* STOCK LIST */}
                <div className="lg:col-span-2">
                    <Card className="h-full">
                        <CardHeader title="Market Overview" subtitle="Real-time sorted view (AVL Tree)" />
                        <div className="overflow-x-auto">
                            <table className="w-full text-left border-collapse">
                                <thead>
                                    <tr className="text-xs text-gray-500 uppercase tracking-wider border-b border-white/10">
                                        <th className="pb-3 pl-2">Symbol</th>
                                        <th className="pb-3 text-right">Price</th>
                                        <th className="pb-3 text-right">Gain/Loss</th>
                                        <th className="pb-3 text-right">RSI</th>
                                        <th className="pb-3 text-right hidden md:table-cell">Holdings</th>
                                        <th className="pb-3 text-right">Action</th>
                                    </tr>
                                </thead>
                                <tbody className="text-sm">
                                    {stocks.length === 0 ? (
                                        <tr><td colSpan={6} className="text-center py-8 text-gray-500">No stocks in portfolio</td></tr>
                                    ) : (
                                        stocks.map((s) => (
                                            <tr key={s.name} className="border-b border-white/5 hover:bg-white/5 transition-colors group">
                                                <td className="py-4 pl-2 font-semibold">{s.name}</td>
                                                <td className="py-4 text-right">₹{s.currentPrice.toFixed(2)}</td>
                                                <td className={`py-4 text-right font-mono ${s.percentGain >= 0 ? 'text-emerald-400' : 'text-red-400'}`}>
                                                    {s.percentGain >= 0 ? '+' : ''}{s.percentGain.toFixed(2)}%
                                                </td>
                                                <td className="py-4 text-right">
                                                    <span className={`px-2 py-1 rounded text-xs ${s.rsi < 30 ? 'bg-emerald-500/20 text-emerald-300' : s.rsi > 70 ? 'bg-red-500/20 text-red-300' : 'text-gray-400'}`}>
                                                        {s.rsi.toFixed(1)}
                                                    </span>
                                                </td>
                                                <td className="py-4 text-right hidden md:table-cell text-gray-400">{s.quantity}</td>
                                                <td className="py-4 text-right">
                                                    <button onClick={() => { setUpdateName(s.name); }} className="text-xs text-[#7f5af0] hover:underline opacity-0 group-hover:opacity-100 transition-opacity">
                                                        Edit
                                                    </button>
                                                </td>
                                            </tr>
                                        ))
                                    )}
                                </tbody>
                            </table>
                        </div>
                    </Card>
                </div>
            </div>

            {/* DSA INFO SECTION */}
            <div className="mb-8">
                <CorrelationGraph />
            </div>

            <div className="mb-12">
                <DSAInfo />
            </div>

            {/* MODAL TO ADD STOCK (Alternative to inline) */}
            {showAddModal && (
                <div className="fixed inset-0 bg-black/80 backdrop-blur-sm flex items-center justify-center z-50 p-4">
                    <Card className="w-full max-w-md bg-[#1e1c2e] border-2 border-[#7f5af0]/50 shadow-[0_0_50px_rgba(127,90,240,0.2)]">
                        <div className="flex justify-between mb-6">
                            <h3 className="text-xl font-bold">Add New Stock</h3>
                            <button onClick={() => setShowAddModal(false)} className="text-gray-400 hover:text-white">✕</button>
                        </div>
                        <form onSubmit={handleAddStock} className="space-y-4">
                            <div>
                                <label className="text-xs uppercase text-gray-500 font-bold mb-1 block">Symbol</label>
                                <input className="input-field" placeholder="e.g. GOOGL" value={newName} onChange={e => setNewName(e.target.value)} required autoFocus />
                            </div>
                            <div className="grid grid-cols-2 gap-4">
                                <div>
                                    <label className="text-xs uppercase text-gray-500 font-bold mb-1 block">Price</label>
                                    <input className="input-field" type="number" step="0.01" placeholder="0.00" value={newPrice} onChange={e => setNewPrice(e.target.value)} required />
                                </div>
                                <div>
                                    <label className="text-xs uppercase text-gray-500 font-bold mb-1 block">Units</label>
                                    <input className="input-field" type="number" placeholder="1" value={newQty} onChange={e => setNewQty(e.target.value)} required />
                                </div>
                            </div>
                            <button type="submit" className="w-full btn-primary py-3 text-lg mt-4">Confirm Purchase</button>
                        </form>
                    </Card>
                </div>
            )}
        </div>
    );
}
