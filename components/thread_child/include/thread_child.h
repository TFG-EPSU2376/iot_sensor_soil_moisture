void initThreadNetwork(void);
void initialize_thread(bool rx, bool (*rx_cb)(void));
esp_err_t joinToThread(bool rx, bool (*rx_cb)(void));
void initUdp(void);
void sendUdp(char *message);
void cleanup(void);
void storeOperationalDataset();
void startFastWakeup();