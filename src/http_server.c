#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "parking_state.h"

// ======================================================
static struct tcp_pcb *server_pcb = NULL;

// ======================================================
static void send_response(struct tcp_pcb *tpcb, const char *data) {
    tcp_write(tpcb, data, strlen(data), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);
}

// ======================================================
static err_t http_recv_callback(void *arg,
                                struct tcp_pcb *tpcb,
                                struct pbuf *p,
                                err_t err) {

    if (!p) {
        tcp_close(tpcb);
        return ERR_OK;
    }

    tcp_recved(tpcb, p->tot_len);
    char *req = (char *)p->payload;

    // ---------- ROTA LOCALIZAR 1 ----------
    if (strstr(req, "GET /localizar1")) {
        localizar_vaga1 = true; // Flag tratada no main.c
        send_response(tpcb, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK");
    } 
    // ---------- ROTA LOCALIZAR 2 ----------
    else if (strstr(req, "GET /localizar2")) {
        localizar_vaga2 = true; // Flag tratada no main.c
        send_response(tpcb, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\nOK");
    }
    // ---------- ROTA /status (JSON) ----------
    else if (strstr(req, "GET /status")) {
        char json[256];
        snprintf(json, sizeof(json),
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n\r\n"
            "{"
              "\"vaga1\": {\"ocupada\": %s, \"tempo\": %lu},"
              "\"vaga2\": {\"ocupada\": %s, \"tempo\": %lu}"
            "}",
            vaga1_status.ocupada ? "true" : "false",
            vaga1_status.tempo_ocupada_ms / 1000,
            vaga2_status.ocupada ? "true" : "false",
            vaga2_status.tempo_ocupada_ms / 1000
        );
        send_response(tpcb, json);
    } 
    // ---------- ROTA PRINCIPAL (HTML) ----------
    else if (strstr(req, "GET / ")) {
        const char *html =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n\r\n"
        "<!DOCTYPE html>"
        "<html><head><meta charset='UTF-8'>"
        "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
        "<title>Estacionamento Inteligente</title>"
        "<style>"
        "  body{margin:0; font-family: sans-serif; background:#121212; color:#fff; display:flex; flex-direction:column; align-items:center;}"
        "  .container{ width: 100%; max-width: 500px; padding: 20px; text-align:center; }"
        "  .card{ background:#1e1e1e; padding:20px; border-radius:15px; margin-bottom:20px; border: 1px solid #333; box-shadow: 0 4px 15px rgba(0,0,0,0.5); }"
        "  .status{ font-size: 1.2rem; font-weight: bold; padding: 5px 15px; border-radius: 20px; display: inline-block; margin: 10px 0; }"
        "  .livre{ background: #1b5e20; color: #a5d6a7; }"
        "  .ocupada{ background: #b71c1c; color: #ef9a9a; }"
        "  .timer{ font-size: 2.5rem; font-weight: bold; font-family: monospace; color: #00e5ff; }"
        "  button{ background: #007bff; color:white; border:none; padding:12px 25px; border-radius:8px; font-weight:bold; cursor:pointer; width:100%; margin-top:15px; transition:0.3s; }"
        "  button:active{ transform: scale(0.95); background: #0056b3; }"
        "</style></head><body>"
        "<div class='container'>"
        "  <h1>Estacionamento Inteligente</h1>"
        "  <div class='card'>"
        "    <h2>Vaga 01</h2>"
        "    <div id='status1' class='status'>---</div>"
        "    <div id='tempo1' class='timer'>00:00:00</div>"
        "    <button onclick='localizar(1)'> LOCALIZAR VEÍCULO</button>"
        "  </div>"
        "  <div class='card'>"
        "    <h2>Vaga 02</h2>"
        "    <div id='status2' class='status'>---</div>"
        "    <div id='tempo2' class='timer'>00:00:00</div>"
        "    <button onclick='localizar(2)'> LOCALIZAR VEÍCULO</button>"
        "  </div>"
        "</div>"
        "<script>"
        "  function localizar(id) { fetch('/localizar' + id); }"
        "  function formatarTempo(seg){"
        "    const h=Math.floor(seg/3600), m=Math.floor((seg%3600)/60), s=seg%60;"
        "    return [h,m,s].map(v => String(v).padStart(2,'0')).join(':');"
        "  }"
        "  function atualizar(){"
        "    fetch('/status').then(r => r.json()).then(d => {"
        "      const v1 = d.vaga1, v2 = d.vaga2;"
        "      document.getElementById('status1').innerHTML = v1.ocupada ? 'OCUPADA' : 'LIVRE';"
        "      document.getElementById('status1').className = 'status ' + (v1.ocupada ? 'ocupada' : 'livre');"
        "      document.getElementById('tempo1').innerHTML = formatarTempo(v1.tempo);"
        "      document.getElementById('status2').innerHTML = v2.ocupada ? 'OCUPADA' : 'LIVRE';"
        "      document.getElementById('status2').className = 'status ' + (v2.ocupada ? 'ocupada' : 'livre');"
        "      document.getElementById('tempo2').innerHTML = formatarTempo(v2.tempo);"
        "    });"
        "  }"
        "  setInterval(atualizar, 1000); atualizar();"
        "</script></body></html>";
        send_response(tpcb, html);
    }

    pbuf_free(p);
    tcp_close(tpcb);
    return ERR_OK;
}

// ======================================================
static err_t http_accept_callback(void *arg, struct tcp_pcb *newpcb, err_t err) {
    tcp_recv(newpcb, http_recv_callback);
    return ERR_OK;
}

// ======================================================
void http_server_init(void) {
    server_pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    tcp_bind(server_pcb, IP_ANY_TYPE, 80);
    server_pcb = tcp_listen(server_pcb);
    tcp_accept(server_pcb, http_accept_callback);
    printf("HTTP ativo em http://192.168.4.1\n");
}