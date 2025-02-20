import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';

export default defineConfig({
  plugins: [react()],
  server: {
    host: '0.0.0.0',  // Isso permite que o Vite aceite conexões externas
    port: 3000,        // Defina a porta desejada (3000 é a padrão)
  },
});