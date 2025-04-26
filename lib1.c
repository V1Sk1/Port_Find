#include <stdio.h>
#include <libserialport.h>
#include <string.h>
#include <locale.h> // Для поддержки локализации (русского языка)
#include <unistd.h> // Для usleep()

#define BAUD_RATE 9600
#define PING_COMMAND "PING\n"
#define EXPECTED_RESPONSE "PONG\n"
#define TIMEOUT 1000 // Таймаут в миллисекундах

int main() {
    setlocale(LC_ALL, ".UTF-8"); // Устанавливаем локаль UTF-8 для поддержки русского языка
    struct sp_port **port_list;
    enum sp_return result;
    int i;

    result = sp_list_ports(&port_list); // Получаем список портов
    if (result != SP_OK) {
        fprintf(stderr, "Ошибка получения списка портов.\n");
        return 1;
    }

    for (i = 0; port_list[i] != NULL; i++) {
        struct sp_port *port = port_list[i];
        char *port_name = sp_get_port_name(port);
        printf("Пробуем порт: %s\n", port_name);

        result = sp_open(port, SP_MODE_READ_WRITE); // Открываем порт для чтения/записи
		usleep(2000000); // 2 секунды
        if (result == SP_OK) {
            // Настраиваем порт
            sp_set_baudrate(port, BAUD_RATE);
            sp_set_parity(port, SP_PARITY_NONE);
            sp_set_bits(port, 8);
            sp_set_stopbits(port, 1);

            // Отправляем "PING"
            sp_blocking_write(port, PING_COMMAND, strlen(PING_COMMAND), TIMEOUT);

            // Читаем ответ
            char buffer[256];
            memset(buffer, 0, sizeof(buffer));
            int bytes_read = sp_blocking_read(port, buffer, sizeof(buffer) - 1, TIMEOUT);

            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                printf("Получено от порта %s: %s", port_name, buffer);

                if (strcmp(buffer, EXPECTED_RESPONSE) == 0) {
                    printf("Arduino найден на порту %s!\n", port_name);
                    sp_free_port_list(port_list); // Освобождаем память
                    sp_close(port);
                    return 0; // Выходим из программы, если нашли Arduino
                } else {
                    printf("Неверный ответ.\n");
                }
            } else {
                printf("Таймаут или ошибка при чтении.\n");
            }
            sp_close(port); // Закрываем порт
        } else {
            fprintf(stderr, "Не удалось открыть порт %s\n", port_name);
        }
    }

    sp_free_port_list(port_list); // Освобождаем память
    printf("Arduino не найден ни на одном порту.\n");
    return 1;
}