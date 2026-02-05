"use client";
import { useEffect, useState } from 'react';
import { Card, CardHeader } from './ui-card';
import { Share2, AlertTriangle } from 'lucide-react';

interface Cluster {
    members: string[];
}

export default function CorrelationGraph() {
    const [clusters, setClusters] = useState<Cluster[]>([]);
    const [loading, setLoading] = useState(true);

    const fetchClusters = async () => {
        try {
            const res = await fetch('http://localhost:5000/api/clusters');
            const data = await res.json();
            // Ensure data is array
            if (Array.isArray(data)) {
                setClusters(data);
            } else {
                setClusters([]);
            }
        } catch (error) {
            console.error("Failed to fetch clusters", error);
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        fetchClusters();
        const interval = setInterval(fetchClusters, 5000);
        return () => clearInterval(interval);
    }, []);

    if (clusters.length === 0 && !loading) return null;

    return (
        <Card className="border-t-4 border-t-yellow-500 mb-8">
            <CardHeader
                title="Risk Clusters (Graph Analysis)"
                subtitle="Correlated stocks detecting sector-wide crashes (RSI < 30)"
            />

            {loading ? (
                <div className="p-4 text-gray-500 animate-pulse">Analyzing graph connections...</div>
            ) : clusters.length === 0 ? (
                <div className="p-4 text-gray-400 italic">No significant correlation clusters detected.</div>
            ) : (
                <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
                    {clusters.map((cluster, idx) => (
                        <div key={idx} className="bg-yellow-500/10 border border-yellow-500/30 p-4 rounded-xl flex items-start gap-3">
                            <Share2 className="text-yellow-500 w-6 h-6 mt-1 flex-shrink-0" />
                            <div>
                                <h4 className="font-bold text-yellow-100 mb-1">Crash Cluster #{idx + 1}</h4>
                                <div className="flex flex-wrap gap-2">
                                    {cluster.members.map(member => (
                                        <span key={member} className="px-2 py-1 bg-yellow-500/20 text-yellow-300 text-xs rounded border border-yellow-500/30">
                                            {member}
                                        </span>
                                    ))}
                                </div>
                                <div className="mt-2 flex items-center gap-1 text-xs text-yellow-500/80">
                                    <AlertTriangle className="w-3 h-3" />
                                    <span>High Correlation Risk</span>
                                </div>
                            </div>
                        </div>
                    ))}
                </div>
            )}
        </Card>
    );
}
