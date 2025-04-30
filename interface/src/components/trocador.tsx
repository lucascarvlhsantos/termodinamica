import React from 'react';
import './trocador.css';

const Trocador = () => {
  return (
    <>
        <div className="reservatorio reservatorio-topo"></div>
        <div className="tubo tubo-topo"></div>
        
        <div className="trocador">
            <div className="casco"></div>
            <div className="fluido-casco"></div>
            <div className="tubo-central"></div>
        </div>
    </>
  );
};

export default Trocador;