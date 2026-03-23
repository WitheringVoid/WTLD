import { create } from 'zustand';
import type { WebSocketEvent } from '@/types/api';

interface WebSocketState {
  ws: WebSocket | null;
  isConnected: boolean;
  events: WebSocketEvent[];
  connect: (token: string) => void;
  disconnect: () => void;
  send: (message: Record<string, unknown>) => void;
  clearEvents: () => void;
}

export const useWebSocketStore = create<WebSocketState>((set, get) => ({
  ws: null,
  isConnected: false,
  events: [],
  
  connect: (token: string) => {
    const wsUrl = `ws://localhost:8080/api/ws?token=${token}`;
    const ws = new WebSocket(wsUrl);
    
    ws.onopen = () => {
      set({ isConnected: true });
      console.log('WebSocket connected');
    };
    
    ws.onmessage = (event) => {
      try {
        const data: WebSocketEvent = JSON.parse(event.data);
        set((state) => ({
          events: [...state.events, data],
        }));
      } catch (error) {
        console.error('Failed to parse WebSocket message:', error);
      }
    };
    
    ws.onclose = () => {
      set({ isConnected: false, ws: null });
      console.log('WebSocket disconnected');
    };
    
    ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };
    
    set({ ws });
  },
  
  disconnect: () => {
    const { ws } = get();
    if (ws) {
      ws.close();
      set({ ws: null, isConnected: false });
    }
  },
  
  send: (message: Record<string, unknown>) => {
    const { ws } = get();
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify(message));
    }
  },
  
  clearEvents: () => {
    set({ events: [] });
  },
}));
