import React, { useEffect, useState } from "react";
import { Line } from 'react-chartjs-2';
import { Chart as ChartJS, CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend } from 'chart.js';
import { ChartOptions, ChartData, ChartTypeRegistry } from 'chart.js';
import { color } from "chart.js/helpers";

ChartJS.register(CategoryScale, LinearScale, PointElement, LineElement, Title, Tooltip, Legend);

let pidData = [
    { time: 0, temperature: 0 },
];

const Informations = ({ data }) => {
  const [percentage, setPercentage] = useState(0); // Estado para controlar a porcentagem

  const calculateColor = (temperature, setpoint) => {
    const maxDistance = 14; // Distância máxima (em ºC)
    const distance = Math.abs(temperature - setpoint);
    
    if (distance >= maxDistance) {
      return "#e23d3d"; // Vermelho
    }
    
    if (distance === 0) {
      return "green"; // Verde
    }
    
    // Formula de interpolação simples entre verde (rgb(0, 255, 0)) e vermelho (rgb(255, 0, 0))
    const green = Math.max(0, 255 - (distance / maxDistance) * 255);
    const red = Math.min(255, (distance / maxDistance) * 255);
    
    return `rgb(${red}, ${green}, 0)`; // Interpolação entre verde e vermelho
  };
  
  useEffect(() => {
    setPercentage(data.potenciaBomba);
  }, [data.potenciaBomba]);

  // Opções do gráfico
  const options: ChartOptions<'line'> = {
    responsive: true,
    plugins: {
      title: {
        display: false,
        font: {
          family: 'Poppins, serif',
        },
      },
      legend: {
        display: false, // Remover a legenda no topo
      },
      tooltip: {
        enabled: true, // Ativar tooltips (valores ao passar o mouse)
      },
    },
    scales: {
      x: {
        grid: {
          display: false, // Remove o grid do eixo X
        },
        ticks: {
          color: '#313131', // Cor dos ticks (marcas) no eixo X
          callback: function (value) {
            // Mostrar apenas t = 0 e t = tmax no eixo X
            if (value === pidData[pidData.length - 1].time) {
              return value;
            }
            return '';
          },
        },
        type: 'linear',
        min: pidData[0].time, // Mostrar apenas o valor inicial no eixo X
        max: pidData[pidData.length - 1].time, // Mostrar apenas o valor final no eixo X
      },
      y: {
        grid: {
          display: false, // Remove o grid do eixo Y
        },
        ticks: {
          color: '#313131', // Cor dos ticks (marcas) no eixo Y
          callback: function (value) {
            // Mostrar apenas 0, o valor inicial e o valor final no eixo Y
            if (
              value === 0 ||
              value === pidData[0].temperature ||
              value === pidData[pidData.length - 1].temperature
            ) {
              return value;
            }
            return ''; // Esconde os valores intermediários
          },
        },
        type: 'linear',
        min: 0, // Valor mínimo para o eixo Y
        max: Math.max(...pidData.map((dataPoint) => dataPoint.temperature)), // Valor máximo para o eixo Y
      },
    },
    elements: {
      point: {
        radius: 0, // Remover os pontos da linha
      },
    },
    animation: {
      duration: 0, // Remover animação para uma transição mais rápida
    },
    maintainAspectRatio: false,
    layout: {
      padding: {
        top: 20,
        bottom: 20,
      },
    },
    // Adicionando o evento beforeDraw para aplicar o gradiente
    onHover: (event, chartElements) => {
      if (chartElements.length > 0) {
        // Ativar tooltips ao passar o mouse
        const chart = chartElements[0]._chart;
        chart.tooltip.setActiveElements(chartElements);
        chart.update();
      }
    },
  };

  // Estrutura dos dados para o gráfico
  const chartData: ChartData<'line'> = {
    labels: pidData.map((dataPoint) => dataPoint.time), // Eixo X: tempo
    datasets: [
      {
        label: 'Temperatura',
        data: pidData.map((dataPoint) => dataPoint.temperature), // Eixo Y: temperatura
        borderColor: '#56987d', // Cor da linha (curva)
        backgroundColor: (context) => {
          const chart = context.chart;
          const ctx = chart.ctx;
          const chartArea = chart.chartArea;

          if (!chartArea) {
            return '#1f4b35'; // Retorna uma cor padrão se o chartArea não estiver disponível
          }

          const gradient = ctx.createLinearGradient(0, chartArea.top, 0, chartArea.bottom);
          gradient.addColorStop(0, '#1f4b35'); // Cor inicial
          gradient.addColorStop(1, '#1f2121'); // Cor final
          return gradient;
        },
        fill: true, // Preencher a área abaixo da linha
        tension: 0.1, // Suaviza a linha
      },
    ],
  };

  const cor = calculateColor(data.temperatura, data.setpoint);

  if (pidData.length > 10) {
    pidData = [];
  }

  if (Math.abs(data.setpoint - data.temperatura) < 0.6) {
    const intervalo = (Date.now() - data.tempInicial) / 1000;
    if (intervalo < 10) {

    }
    else {
      pidData.push({time: intervalo, temperature: data.temperatura});
    }
  }

  return (
    <>
      <div className="informations-container">
        <div className="footer-info">
          <div className="temperature">
            <div className="title">
              <h4>Vazão</h4>
            </div>
            <div className="content">
              <h2 style={{color:'#fff'}}>{data.vazao.toFixed(2)}</h2>
              <span style={{color:'#fff'}}>l/min</span>
            </div>
            <div className="setpoint">
              <span>
                Vazão volumétrica <br /> da bomba
              </span>
            </div>
          </div>
          <div className="temperature heat">
            <div className="title">
              <h4>Taxa de calor</h4>
            </div>
            <div className="content">
              <h2 style={{color:'#fff'}}>{data.calorTrocado.toFixed(2) * 1000}</h2>
              <span style={{color:'#fff'}}>w</span>
            </div>
            <div className="setpoint">
              <span>
                Calor trocado <br /> internamente
              </span>
            </div>
          </div>
        </div>
        <div className="bottom-info">
          <div
            className="progress-bar"
            style={{
              fontFamily: '"Poppins", serif',
              border: '1px solid #252525',
              color: '#7f877d',
              display: 'flex',
              flexDirection: 'column',
              borderRadius: '15px',
              padding: '2rem 2.5rem',
              alignItems: 'flex-start',
              marginTop: '2rem',
              width: '78%',
              height: 'calc(100% / 6) !important',
              backgroundColor: percentage === 0 ? '#151614' : percentage === 100 ? '#1a1509' : '#1a1509', // Cor de fundo dinâmica
              background: `linear-gradient(to right, #2f3930 ${percentage}%, #151614 ${percentage}%)`, // Gradiente para o efeito de progresso
            }}
          >
            <div className="top-title">
              <span>Potência da bomba</span>
            </div>
            <div className="number">
              <h2>{percentage}%</h2>
            </div>
          </div>
          <div className="temperature">
            <div className="title">
              <h4>Temperatura</h4>
            </div>
            <div className="content">
              <h2 style={{color: cor}}>{data.temperatura}</h2>
              <span style={{color: cor}}>ºC</span>
            </div>
            <div className="setpoint">
              <span>
                Temperatura na <br /> saída desejada
              </span>
            </div>
          </div>
        </div>
        <div className="graph" style={{marginTop: "3rem"}}>
          <div className="title">
            <h4>Gráfico da temperatura no tempo</h4>
          </div>
          <Line data={chartData} options={options} />
        </div>
      </div>
    </>
  );
};

export default Informations;

