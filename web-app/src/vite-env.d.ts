/// <reference types="vite/client" />
/// <reference types="@types/w3c-web-serial" />
/// <reference types="web-bluetooth" />

interface ImportMetaEnv {
  readonly VITE_FAKE_BLE: string;
}

interface ImportMeta {
  readonly env: ImportMetaEnv;
}
