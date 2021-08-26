#include "tcode.h"
#include "tcode_buffer.h"

#include <ctype.h>
#include <string.h>
#include <math.h>

static tcode_buffer_t _tcode_buffer = TCODE_BUFFER_DEFAULT;
static tcode_buffer_t _pending_buffer = TCODE_BUFFER_DEFAULT;

// todo: autogenerate this
const char *tcode_command_type_str[5] = {
    "TCODE_COMMAND_INVALID",
    "TCODE_COMMAND_LINEAR",
    "TCODE_COMMAND_ROTATE",
    "TCODE_COMMAND_VIBRATE",
    "TCODE_COMMAND_AUXILLIARY",
};

tcode_err_t tcode_run_buffer(tcode_command_callback_t cb, long ms) {
    tcode_buffer_node_t *node = _tcode_buffer.first;
    size_t idx = 0;

    while (node != NULL) {
        tcode_command_t *cmd = &node->command;
        node = node->next;

        tcode_command_t command = TCODE_COMMAND_DEFAULT;
        command.type = cmd->type;
        command.magnitude = cmd->magnitude;
        cb(command);

        if (cmd->interval <= 0.0 && cmd->speed <= 0.0) {
            tcode_buffer_remove_idx(&_tcode_buffer, idx);
        } else {
            cmd->magnitude += cmd->transition_magnitude;
        }

        idx++;
    }

    return TCODE_OK;
}

tcode_err_t tcode_send_command(FILE* stream, tcode_command_t* cmd) {
    return TCODE_FAIL;
}

tcode_err_t tcode_queue_command(FILE* stream, tcode_command_t* cmd) {
    return TCODE_FAIL;
}

tcode_err_t tcode_flush_queue(void) {
    return TCODE_FAIL;
}

tcode_err_t tcode_queue_commands(FILE* stream, tcode_command_t** cmds, size_t cmds_len) {
    return TCODE_FAIL;
}

tcode_err_t tcode_send_commands(FILE* stream, tcode_command_t** cmds, size_t cmds_len) {
    return TCODE_FAIL;
}

tcode_err_t tcode_parse_str(const char* buffer, tcode_command_t* cmd) {
    if (strlen(buffer) < 3) {
        return TCODE_FAIL;
    }

    size_t idx = 0;

    switch (toupper(buffer[idx++])) {
    case 'A':
        cmd->type = TCODE_COMMAND_AUXILLIARY;
        break;
    case 'V':
        cmd->type = TCODE_COMMAND_VIBRATE;
        break;
    case 'R':
        cmd->type = TCODE_COMMAND_ROTATE;
        break;
    case 'L':
        cmd->type = TCODE_COMMAND_LINEAR;
        break;
    default:
        return TCODE_FAIL;
    }

    if (!isdigit(buffer[idx])) {
        return TCODE_FAIL;
    }

    cmd->channel = buffer[idx++] - '0';

    enum {
        _NUMERIC_MAGNITUDE,
        _NUMERIC_SPEED,
        _NUMERIC_INTERVAL,
    } target = _NUMERIC_MAGNITUDE;

    float magnitude_order = 1.0;

    while (buffer[idx] != '\0') {
        if (isdigit(buffer[idx])) {
            if (target == _NUMERIC_INTERVAL) {
                cmd->interval *= 10;
                cmd->interval += (buffer[idx] - '0');
            } else {
                float diff = ((float) (buffer[idx] - '0')) / powf(10.0, magnitude_order);
                magnitude_order += 1.0;

                if (target == _NUMERIC_MAGNITUDE) {
                    cmd->magnitude += diff;
                } else if (target == _NUMERIC_SPEED) {
                    cmd->speed += diff;
                }
            }
        } 

        else {
            magnitude_order = 1.0;

            switch (toupper(buffer[idx])) {
            case 'S':
                target = _NUMERIC_SPEED;
                break;
            case 'I':
                target = _NUMERIC_INTERVAL;
                break;
            default:
                return TCODE_FAIL;
            }
        }

        idx++;
    }

    // Finally, calculate transition data.

    if (cmd->interval > 0.0) {
        // interval is how many ms to finish the transition time in 
        // which is actually shitty because now we need to keep state of
        // every prior channel and i quit
    }

    if (cmd->speed > 0.0) {
        // speed is n per 100ms, so
        // S10 = 10 / 100ms, 
    }

    return TCODE_OK;
}

tcode_err_t tcode_parse_stream(FILE* stream, tcode_text_buffer_t* buffer, tcode_data_callback_t cb, char* response, size_t resplen) {
    return TCODE_OK;
}

size_t tcode_parse_buffer(tcode_text_buffer_t* buffer, tcode_data_callback_t cb, char* response, size_t resplen) {
    tcode_command_t cmd = TCODE_COMMAND_DEFAULT;

    size_t buffer_len = 0;
    if (buffer->head < buffer->tail) {
        buffer_len = (buffer->head + TCODE_TEXT_BUFFER_LEN) - buffer->tail;
    } else {
        buffer_len = buffer->head - buffer->tail;
    }

    if (buffer_len <= 0) return 0;

    char flat[TCODE_TEXT_BUFFER_LEN];
    tcode_err_t token_err = TCODE_FAIL;

    for (size_t token_len = 0; token_len < buffer_len; token_len++) {
        size_t idx = (buffer->tail + token_len) % TCODE_TEXT_BUFFER_LEN;
        char c = buffer->buffer[idx];

        if (c == ' ' || c == '\n') {
            flat[token_len] = '\0';

            if (c == '\n') {
                cmd.execute_immediate = 1;
            }

            token_err = tcode_parse_str(flat, &cmd);
            buffer->tail = (idx + 1) % TCODE_TEXT_BUFFER_LEN;
            break;
        }

        else {
            flat[token_len] = c;
        }
    }

    if (TCODE_OK == token_err) {
        printf("type=%s, channel=%d, magnitude=%f, speed=%f, interval=%f\n", 
            tcode_command_type_str[cmd.type], 
            cmd.channel,
            cmd.magnitude,
            cmd.speed,
            cmd.interval
        );

        tcode_buffer_enqueue(&_pending_buffer, &cmd);
    }

    if (cmd.execute_immediate) {
        tcode_buffer_copy(&_tcode_buffer, &_pending_buffer);
        tcode_buffer_empty(&_pending_buffer);
    }

    return 0;
}

tcode_err_t tcode_getc(FILE* stream, tcode_text_buffer_t* buffer) {
    char c = getc(stream);

    while (c != 0xFF) {
        buffer->buffer[buffer->head] = c;
        buffer->head = (buffer->head + 1) % TCODE_TEXT_BUFFER_LEN;
        if (buffer->head == buffer->tail) return TCODE_BUFFER_OVERFLOW;
        c = getc(stream);
    }

    return TCODE_OK;
}