export function Card({ children, className = "" }: { children: React.ReactNode; className?: string }) {
    return (
        <div className={`glass-card p-6 ${className}`}>
            {children}
        </div>
    );
}

export function CardHeader({ title, subtitle }: { title: string; subtitle?: string }) {
    return (
        <div className="mb-6">
            <h3 className="text-xl font-bold bg-clip-text text-transparent bg-gradient-to-r from-purple-400 to-pink-600">
                {title}
            </h3>
            {subtitle && <p className="text-sm text-gray-400 mt-1">{subtitle}</p>}
        </div>
    );
}
