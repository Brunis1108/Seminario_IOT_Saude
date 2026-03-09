# Seminario_IOT_Saude

Projeto acadêmico de **IoT aplicada à saúde** desenvolvido com a **BitDogLab / Raspberry Pi Pico W (RP2040)**, com foco em **monitoramento de temperatura** e **detecção de possíveis quedas**.

O sistema utiliza sensores embarcados para coletar dados físicos do ambiente/paciente e processa essas informações localmente, permitindo a exibição do estado do monitoramento durante a simulação prática do seminário.



## Objetivo do projeto

Este projeto foi desenvolvido como uma **simulação prática de monitoramento em IoT na saúde**, com a finalidade de demonstrar como sensores conectados podem ser usados no acompanhamento remoto de condições relevantes ao cuidado de pacientes, especialmente:

- leitura de temperatura;
- detecção de movimento brusco;
- identificação de possível queda;
- exibição do estado do sistema para fins didáticos e demonstrativos.



## Funcionalidades

- Conexão Wi-Fi com a **Pico W**
- Leitura de temperatura com o sensor **DS18B20**
- Leitura de aceleração, rotação e orientação com o **MPU6050**
- Cálculo de:
  - aceleração total;
  - rotação total;
  - roll;
  - pitch.
- Detecção heurística de:
  - **movimento brusco**
  - **possível queda**
- Exibição dos dados no **monitor serial**
- Uso de botões físicos da placa:
  - botão A para teste/debug;
  - botão B para entrar em modo **BOOTSEL**

---

## Hardware utilizado

- **BitDogLab / Raspberry Pi Pico W**
- **Sensor DS18B20**
- **Sensor MPU6050**
- Cabos jumper
- Alimentação via USB



## Mapeamento de pinos

### DS18B20
- **GPIO 4**

### MPU6050 (I2C0)
- **SDA:** GPIO 0
- **SCL:** GPIO 1

### Botões
- **Botão A (usuário/debug):** GPIO 5
- **Botão B (BOOTSEL):** GPIO 6



## Lógica do sistema

O projeto realiza leituras periódicas dos sensores e classifica o estado do monitoramento com base em regras simples.

### Temperatura
A temperatura corporal estimada é obtida com o **DS18B20**.

### Movimento e orientação
O **MPU6050** fornece:
- aceleração nos eixos X, Y e Z;
- velocidade angular nos eixos X, Y e Z;
- temperatura interna do sensor;
- estimativa de inclinação por meio de **roll** e **pitch**.

### Regras de alerta

O sistema considera:

- **Movimento brusco** quando há rotação elevada ou grande variação na aceleração;
- **Possível queda** quando ocorre pico de aceleração acompanhado de mudança significativa de orientação.

De forma resumida, a lógica usada na simulação foi:

- **Normal:** leituras dentro do comportamento esperado;
- **Alerta de movimento brusco:** rotação/aceleração fora do padrão;
- **Alerta de possível queda:** impacto + mudança brusca de inclinação.



## Saída do sistema

Durante a execução, o sistema imprime no terminal serial informações como:

- tempo de execução;
- temperatura do MPU6050;
- temperatura do DS18B20;
- aceleração nos três eixos;
- giroscópio nos três eixos;
- orientação (roll e pitch);
- status atual do monitoramento.

Exemplo de status:

- `NORMAL`
- `ALERTA: MOVIMENTO BRUSCO`
- `ALERTA: POSSIVEL QUEDA`



## Estrutura do projeto

```text
IotSaudePico/
├── CMakeLists.txt
├── Saude.c
├── pico_sdk_import.cmake
├── lib/
│   ├── ds18b20.c
│   ├── mpu6050.c
│   └── headers/
├── build/
│   ├── Saude.uf2
│   ├── Saude.elf
│   ├── Saude.bin
│   └── ...
└── .vscode/
```
## Requisitos para compilação

Para compilar o projeto, você precisa ter instalado:

- Raspberry Pi Pico SDK
- CMake
- Ninja ou Make
- ARM GCC Toolchain
- VS Code com extensão da Raspberry Pi Pico (opcional, mas recomendado)

Como compilar
1. Clone o repositório
git clone https://github.com/Brunis1108/Seminario_IOT_Saude.git
cd Seminario_IOT_Saude
2. Crie a pasta de build
mkdir build
cd build
3. Gere os arquivos do projeto
cmake ..
4. Compile
cmake --build .

Ao final da compilação, o arquivo gerado para gravação na placa será:
- build/Saude.uf2

Como gravar na placa

1. Desconecte a placa do USB.
2. Pressione e segure o botão BOOTSEL da Pico W.
3. Conecte a placa ao computador.
4. Solte o botão.
5. Copie o arquivo Saude.uf2 para a unidade montada da placa.

## Execução e monitoramento

Após gravar o firmware:

- abra o monitor serial;
- aguarde a inicialização do sistema;
- observe as leituras dos sensores;
- simule mudanças de posição e movimentos bruscos;
- acompanhe a alteração do status no terminal.

## Wi-Fi

O projeto utiliza conexão Wi-Fi com credenciais definidas diretamente no código-fonte:

- #define WIFI_SSID "SEU SSID"
- #define WIFI_PASSWORD "SUA SENHA"

Antes de usar em outro ambiente, ajuste essas credenciais no arquivo Saude.c.

## Observações importantes

Este projeto tem finalidade acadêmica e demonstrativa. A detecção de queda foi implementada com base em uma heurística simples, não em um algoritmo clínico validado. O sistema não substitui dispositivos médicos ou soluções profissionais de monitoramento. Para uso real, seriam necessários testes mais robustos, calibração adequada e validação técnica.

## Autoria

Projeto desenvolvido para apresentação acadêmica na disciplina/seminário sobre IoT na Saúde.

