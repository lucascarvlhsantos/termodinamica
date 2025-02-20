import './App.css'
import Informations from './components/informations'
import Topbar from './components/topbar'
import Trocador from './components/trocador'
import { useState, useEffect } from 'react'

const vazaoMaxima = 1.9986;
const Cp = 1;
const socket = new WebSocket("ws://192.168.43.10:81");
const prevTemp = Date.now() / 1000;

function App() {
  const [value, setValue] = useState('');
  const [data, setData] = useState({
    temperatura: 0,
    potenciaBomba: 0,
    vazao: 0,
    calorTrocado: 0,
    setpoint: 0,
    tempInicial: prevTemp,
    tempAtual: 0
  });

  const vazao = (potenciaBomba: number) => {
    return vazaoMaxima * potenciaBomba / 100; 
  }

  const calorTrocado = (temperaturaAmbiente: number, temperaturaNaSaida: number, potenciaBomba: number) => {
    const vazaoEmKgPorS = vazao(potenciaBomba) * 0.001 / 60;
    const densidade = 1000 - (0.07 * (temperaturaNaSaida - 20));
    const vazaoMassica = densidade * vazaoEmKgPorS;
    return vazaoMassica * Cp * (temperaturaNaSaida - 6);
  }

  const handleChange = (e) => {
    let inputValue = e.target.value;

    if (inputValue === '') {
      inputValue = '';
    } else {
      // Se o valor for menor que 10 e tem um único dígito, adiciona o zero à esquerda
      if (inputValue < 10 && inputValue.length === 1) {
        inputValue = `0${inputValue}`;
      } else {
        inputValue = inputValue.replace(/^0+/, '');
      }
    }

    if (inputValue.length > 2) {
      inputValue = inputValue.slice(0, 2);
    }
    setValue(inputValue);
  };

  // WEB SOCKET
  const sendSetpoint = () => {
    if (value > 0 && value <= 30) {
      socket.send(JSON.stringify({ setpoint: value }));
      console.log(`Setpoint enviado: ${value}`);
    } else {
      console.log("O setpoint deve ser entre 1 e 30 ºC.");
    }
  };

  useEffect(() => {
    const socket = new WebSocket("ws://192.168.43.10:81");
  
    socket.onopen = () => {
      console.log("Conectado ao WebSocket");
    };
  
    socket.onmessage = (event) => {
      console.log("Mensagem recebida:", event.data);
      try {
        const receivedData = JSON.parse(event.data);
        const vazaoCalculada = vazao(receivedData.potenciaBomba);
        const calorCalculado = calorTrocado(receivedData.temperaturaAmbiente, receivedData.temperatura, receivedData.potenciaBomba);
  
        setData((prevData) => ({
          ...prevData,
          ...receivedData,
          vazao: vazaoCalculada,
          calorTrocado: calorCalculado,
          tempAtual: Date.now() / 1000
        }));
      } catch (error) {
        console.error("Erro ao processar os dados do WebSocket:", error);
      }
    };
  
    socket.onclose = () => {
      console.log("Desconectado do WebSocket");
    };
  
    socket.onerror = (error) => {
      console.error("Erro no WebSocket:", error.message);
    };
  
    // Buscar dados a cada 2 segundos
    const interval = setInterval(() => {
      console.log("Buscando dados do servidor...");
      socket.send(JSON.stringify({ ping: true }));
    }, 1000);
  
    // Limpar interval e fechar a conexão quando o componente for desmontado
    return () => {
      clearInterval(interval);
      socket.close();
    };
  }, []);

  return (
    <>
    <Topbar />
      <div className='main'>
        <div className='left-container'>
          <div className='trocador-content'>
            <div className='setpoint-set'>
              <div className='setpoint-title'>
                <span>Mudar setpoint</span>
                <div className='setpoint-value'>
                  <input 
                  type="number" 
                  value={value} 
                  onInput={handleChange} 
                  min="0" 
                  max="99" 
                  />
                  <span>ºC</span>
                </div>
              </div>
              <div className='setpoint-content'>
                <span>Setpoint atual</span>
                <div className='setpoint-atual'>
                  <h4>{data.setpoint}</h4>
                  <span>ºC</span>
                </div>
              </div>
              <button onClick={sendSetpoint}>Atualizar</button>
            </div>
            <Trocador />
          </div>
        </div>
        <div className='right-container'>
        <Informations data={data} />
        </div>
      </div>
    </>
  )
}

export default App
