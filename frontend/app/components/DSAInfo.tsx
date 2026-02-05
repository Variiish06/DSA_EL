import { Card, CardHeader } from "./ui-card";
import { Database, GitBranch, Layers, Activity } from 'lucide-react';

export default function DSAInfo() {
    return (
        <Card className="h-full border-blue-500/20">
            <CardHeader title="DSA Engine Architecture" subtitle="Powered by Optimized C Backend" />

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
                <div className="flex items-start gap-3 p-3 rounded-xl bg-white/5 hover:bg-white/10 transition-colors">
                    <Database className="w-6 h-6 text-purple-400 mt-1" />
                    <div>
                        <h4 className="font-semibold text-purple-200">Hash Table</h4>
                        <p className="text-xs text-gray-400 mt-1">O(1) average time complexity for stock lookups by name.</p>
                    </div>
                </div>

                <div className="flex items-start gap-3 p-3 rounded-xl bg-white/5 hover:bg-white/10 transition-colors">
                    <GitBranch className="w-6 h-6 text-pink-400 mt-1" />
                    <div>
                        <h4 className="font-semibold text-pink-200">AVL Tree</h4>
                        <p className="text-xs text-gray-400 mt-1">Self-balancing BST used to maintain stocks sorted by name/gain for efficient in-order traversal.</p>
                    </div>
                </div>

                <div className="flex items-start gap-3 p-3 rounded-xl bg-white/5 hover:bg-white/10 transition-colors">
                    <Layers className="w-6 h-6 text-amber-400 mt-1" />
                    <div>
                        <h4 className="font-semibold text-amber-200">Max/Min Heaps</h4>
                        <p className="text-xs text-gray-400 mt-1">Dual priority queues to instantly access Top Gainer and Top Loser in O(1).</p>
                    </div>
                </div>

                <div className="flex items-start gap-3 p-3 rounded-xl bg-white/5 hover:bg-white/10 transition-colors">
                    <Activity className="w-6 h-6 text-emerald-400 mt-1" />
                    <div>
                        <h4 className="font-semibold text-emerald-200">Circular Queue + BIT</h4>
                        <p className="text-xs text-gray-400 mt-1">Ring buffer for price history effeciently coupled with Fenwick Trees for O(log N) indicator calculations.</p>
                    </div>
                </div>
            </div>
        </Card>
    );
}
