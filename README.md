<h1 align="left">Estacionamento Inteligente com RP2040</h1>
<p align="left"> Este projeto foi desenvolvido como requisito para a disciplina de <strong>Arquitetura de Computadores (2025.2)</strong>.<br> Trata-se de um protótipo de <strong>estacionamento inteligente</strong> desenvolvido com o microcontrolador <strong>RP2040</strong>, utilizando a plataforma educacional <strong>BitDogLab (Raspberry Pi Pico W)</strong>. </p>
<h2 align="left">Contexto Acadêmico</h2>
<p align="left"> Disciplina: Arquitetura de Computadores<br> Período: 2025.2<br> Plataforma: BitDogLab / Raspberry Pi Pico W (RP2040) </p>
<h2 align="left">Hardware e Componentes</h2>
<p align="left"> <strong>Componentes Externos</strong><br><br>

<strong>Sensor Ultrassônico (HC-SR04)</strong><br>
Utilizado no monitoramento da Vaga 2, realizando medições de distância por pulsos ultrassônicos.<br><br>

<strong>Sensor de Distância ToF (VL53L0X)</strong><br>
Responsável pelo monitoramento de alta precisão da Vaga 1, utilizando tecnologia Time of Flight (laser).<br><br>

<strong>Servomotor (SG90)</strong><br>
Atuador mecânico responsável pelo controle físico da cancela de entrada e saída de veículos.<br><br>

<strong>Protoboard e Jumpers</strong><br>
Utilizados para realizar as interconexões elétricas entre os periféricos e a placa BitDogLab.

</p>
<h2 align="left">Recursos Embutidos na BitDogLab</h2>
<p align="left"> <strong>Display OLED (SSD1306)</strong><br> Interface visual responsável por exibir, em tempo real, o status das vagas e mensagens do sistema.<br><br>

<strong>Buzzers Piezolétricos</strong><br>
Empregados no sistema de alerta sonoro, simulando sensor de ré e localização do veículo.<br><br>

<strong>LEDs RGB / Matriz de LEDs</strong><br>
Indicadores visuais para estado das vagas, permissão de acesso e status geral do sistema.<br><br>

<strong>Wi-Fi (Pico W)</strong><br>
Utilizado para hospedar um servidor HTTP, permitindo monitoramento e controle remoto do estacionamento via navegador.

</p>
