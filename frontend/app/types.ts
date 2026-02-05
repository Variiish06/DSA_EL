export interface Stock {
    name: string;
    buyPrice: number;
    currentPrice: number;
    quantity: number;
    percentGain: number;
    sma: number;
    rsi: number;
    upperAlert: number;
    lowerAlert: number;
}

export interface Transaction {
    type: string;
    symbol: string;
    price: number;
}

export interface Summary {
    totalInvestment: number;
    currentValue: number;
    profit: number;
    stockCount: number;
}
