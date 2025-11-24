🐾 PetControl – Sistema Completo de Gerenciamento para Pet Shops


O PetControl é um ecossistema desenvolvido para auxiliar pet shops em suas rotinas internas e online.
Ele é composto por duas plataformas integradas:

🧩 1) PetControl Desktop – Gerenciamento de Planos (C + Raylib + SQLite)

Uma ferramenta robusta para controle interno dos planos comprados pelo site, oferecendo consultas rápidas, relatórios e envios automáticos de alertas de vencimento.

✨ Funcionalidades

🔍 Consulta de cadastros feitos pelo site

📊 Exportação de relatórios em CSV

✉️ Envio automático de e-mails para planos próximos do vencimento

🗄 Banco de dados local SQLite3

🔐 Criação automática dos arquivos .db (não enviados ao GitHub)

📁 Estrutura do Projeto Desktop
PetControl/
 ├── src/
 │   ├── main.c
 │   ├── painel.c
 │   ├── shell.c
 │   ├── sqlite3.c
 │   ├── sqlite3.h
 │   ├── build.bat
 │
 ├── assets/
 │   └── logo.png
 │
 ├── .gitignore
 ├── README.md

🔧 Como Compilar (Windows)

Dependências utilizadas:

Raylib 5.0 (Win64)

GCC WinLibs (mingw-w64)

SQLite3

Compile executando:

./src/build.bat


O script gera:

PetControl.exe

🧩 2) PetControl Web – Plataforma Online (Java + MySQL + Front-end)

Sistema completo para pet shops com agendamentos, PDV, estoque, pets, clientes e relatórios.

✨ Funcionalidades Principais

📅 Agenda Inteligente

💰 PDV com emissão de notas

🐶 Cadastro e histórico dos pets

👥 Gestão completa de clientes

📦 Controle de estoque com alertas

📊 Relatórios de desempenho

🌐 Interface rápida e responsiva

🌐 Demonstração Online

Acesse o site:

👉 https://luanasacutti.github.io/PetControl/

Abra o site:
open index.html

👩‍💻 Equipe Desenvolvedora

Guilherme Almeida	Back-end & Database
Luana Sacutti	Full-stack Development
Maria Eduarda Ferraz	Front-end & Design

📞 Contato
Desenvolvido com ❤️ pela equipe PetControl.