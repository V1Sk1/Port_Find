#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libserialport.h>
#include <unistd.h> // Для usleep()

#define BAUD_RATE 9600
#define TIMEOUT 1000      // миллисекунды
#define PING_COMMAND "PING\n"
#define EXPECTED_RESPONSE "PONG\n"

// Функция для отправки команды и получения ответа
int send_command_and_receive_response(struct sp_port *port, const char *command, char *buffer, size_t buffer_size) {
    enum sp_return result;
    int bytes_read;

    // Очищаем буфер перед отправкой
    sp_flush(port, SP_BUF_BOTH);

    // Отправляем команду
    result = sp_blocking_write(port, command, strlen(command), TIMEOUT);
    if (result < 0) {
        fprintf(stderr, "Ошибка при отправке команды: %d\n", result);
        return -1; // Ошибка отправки
    }
    printf("Отправлено: %s", command);

    // Читаем ответ
    memset(buffer, 0, buffer_size);
    bytes_read = sp_blocking_read(port, buffer, buffer_size - 1, TIMEOUT);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';
        printf("Получено: %s", buffer);
        return 0; // Успешное получение ответа
    } else {
        printf("Таймаут или ошибка при чтении.\n");
        return -2; // Ошибка чтения
    }
}


struct sp_port *find_arduino_port() {
    struct sp_port **port_list;
    enum sp_return result = sp_list_ports(&port_list);
    if (result != SP_OK) {
        fprintf(stderr, "Ошибка получения списка портов: %d\n", result);
        return NULL;
    }

    struct sp_port *arduino_port = NULL;
    char buffer[256];

    for (int i = 0; port_list[i] != NULL; i++) {
        struct sp_port *port = port_list[i];
        const char *port_name = sp_get_port_name(port);
        printf("Пробуем порт: %s\n", port_name);

        // Открываем порт
        if (sp_open(port, SP_MODE_READ_WRITE) != SP_OK) {
            fprintf(stderr, "Не удалось открыть порт %s\n", port_name);
            continue; // Переходим к следующему порту
        }

        // Настраиваем порт
        sp_set_baudrate(port, BAUD_RATE);
        sp_set_parity(port, SP_PARITY_NONE);
        sp_set_bits(port, 8);
        sp_set_stopbits(port, 1);
        sp_set_flowcontrol(port, SP_FLOWCONTROL_NONE);

        // Даем Arduino время на инициализацию
        usleep(2000000);

        // Проверяем, отвечает ли Arduino на команду PING
        if (send_command_and_receive_response(port, PING_COMMAND, buffer, sizeof(buffer)) == 0) {
            if (strstr(buffer, EXPECTED_RESPONSE) != NULL) { // Используем strstr
                printf("Нашли Arduino на порту: %s\n", port_name);
                arduino_port = port;
                break; // Выходим из цикла, если Arduino найден
            } else {
                printf("Неверный ответ.\n");
            }
        }

        // Закрываем порт, если это не Arduino
        sp_close(port);
    }

    sp_free_port_list(port_list); // Освобождаем память
    return arduino_port;
}


int main() {
    struct sp_port *port = NULL;

    // 1. Ищем порт Arduino
    port = find_arduino_port();
    if (port == NULL) {
        fprintf(stderr, "Arduino не найден ни на одном порту.\n");
        return 1;
    }

    // Дальше работаем с найденным портом
    enum sp_return result;
    int bytes_read;
    char buffer[256];


    // 6. Отправляем "PING\n" и получаем "PONG\n"
    if (send_command_and_receive_response(port, PING_COMMAND, buffer, sizeof(buffer)) == 0) {
        printf("Успешно получили ответ: %s", buffer);
    } else {
        fprintf(stderr, "Ошибка обмена данными с Arduino.\n");
    }


    sp_close(port);
    sp_free_port(port);

    return 0;
}