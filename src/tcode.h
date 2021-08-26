#ifndef __tcode_h
#define __tcode_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

#ifndef TCODE_CMD_CHAR_BUFFER_LEN
#define TCODE_CMD_CHAR_BUFFER_LEN 40
#endif

#ifndef TCODE_TEXT_BUFFER_LEN
#define TCODE_TEXT_BUFFER_LEN 128
#endif

typedef enum tcode_err {
    TCODE_OK,
    TCODE_FAIL,
    TCODE_NO_DATA,
    TCODE_BUFFER_OVERFLOW,
} tcode_err_t;

/**
 * T-Code is an ASCII format. We want to be able to declare a T-Code handler for a given buffer.
 * Should we manually pass in an ASCII buffer or should we build it up like we do with i2c commands?
 * 
 *  tcode_cmd_handle_t cmd = tcode_start(void);
 *  tcode_linear_cmd(cmd, (tcode_channel_t) 2, (tcode_magnitude_t) 77);
 *  tcode_rotate_cmd(cmd, (tcode_channel_t) 0, (tcode_magnitude_t) 90);
 *  tcode_vibrate_cmd(cmd, (tcode_channel_t) 3, (tcode_magnitude_t) 17);
 *  tcode_err_t err = tcode_write_stream(cmd, *stream);
 *  tcode_end(void);
 * 
 * Alternatively, we can queue up commands to write directly to an output stream. This stream can be
 * UART, Bluetooth, or anything else. The annoying thing to handle here is bidirectional commands.
 * We're not gonna talk about those yet. Let's focus on the basic command strucutre:
 */

typedef enum tcode_command_type {
    TCODE_COMMAND_INVALID,
    TCODE_COMMAND_LINEAR,
    TCODE_COMMAND_ROTATE,
    TCODE_COMMAND_VIBRATE,
    TCODE_COMMAND_AUXILLIARY,
} tcode_command_type_t;

extern const char *tcode_command_type_str[5];

typedef struct tcode_command {
    tcode_command_type_t type;
    uint8_t              channel;
    float                magnitude;
    float                interval;
    float                speed;
    float                transition_magnitude;
    long                 last_transition_ms;
    uint8_t              execute_immediate;
} tcode_command_t;

#define TCODE_COMMAND_DEFAULT { \
    .type = TCODE_COMMAND_INVALID, \
    .channel = 0, \
    .magnitude = 0.0, \
    .interval = 0.0, \
    .speed = 0.0, \
    .transition_magnitude = 0.0, \
    .last_transition_ms = 0, \
    .execute_immediate = 0, \
}

/**
 * Some commands are actually for data, not sex. We'll try to handle those:
 */

typedef enum tcode_data_command_type {
    TCODE_DATA_COMMAND_GETCAPS,
} tcode_data_command_type_t;

/** 
 * Given that, we can start some vibrating with something like this, perhaps:
 * 
 *  tcode_command_t cmd = {
 *      .type = TCODE_COMMAND_VIBRATE,
 *      .channel = 1,
 *      .magnitude = 0.99,
 *  };
 */

tcode_err_t tcode_send_command(FILE* stream, tcode_command_t *cmd);

/** 
 * We could also queue the command then execute it at a precise time with the following:
 */

tcode_err_t tcode_queue_command(FILE* stream, tcode_command_t *cmd);
tcode_err_t tcode_flush_queue(void);
 
/** 
 * This would let us do something like the following:
 * 
 *  tcode_queue_command(&stream, &cmd1);
 *  tcode_queue_command(&stream, &cmd2);
 *  tcode_flush_queue();
 * 
 * But for syntax brevity, we'll allow another method that queues n commands, as such:
 * 
 *  tcode_command_t cmds[2] = {
 *      {
 *          .type = TCODE_COMMAND_VIBRATE,
 *          .channel = 1,
 *          .magnitude = 0.99,
 *      },
 *      {
 *          .type = TCODE_COMMAND_LINEAR,
 *          .channel = 1,
 *          .magnitude = 0.5,
 *      },
 *  };
 */

tcode_err_t tcode_queue_commands(FILE* stream, tcode_command_t **cmds, size_t cmds_len);
 
/** 
 * The same can be used to immediately execute the command order, without calling Flush:
 */

tcode_err_t tcode_send_commands(FILE* stream, tcode_command_t **cmds, size_t cmds_len);

/**
 * And now we can read things. First, we might end up with some form of callback that requires 
 * a response, from the D commands. I'll call them Data. Data is neat.
 */

typedef void (tcode_data_callback_t)(tcode_data_command_type_t cmd, FILE* ostream);

/** 
 * Given this, we can now write to some form of output stream that is consistent with the POSIX
 * FILE* handle. But what do we do with such a stream if we're the device, not the sender? Consider:
 * 
 *  tcode_command_t cmd;
 *  while(tcode_parse_stream(&stream, &cmd) == 0) {
 *      // do something with cmd here
 *  }
 */

typedef struct tcode_text_buffer {
    char buffer[TCODE_TEXT_BUFFER_LEN];
    size_t head;
    size_t tail;
} tcode_text_buffer_t;

#define TCODE_TEXT_BUFFER_DEFAULT { \
    .buffer = "", \
    .head = 0, \
    .tail = 0, \
}

tcode_err_t tcode_getc(FILE* stream, tcode_text_buffer_t *buffer);
size_t tcode_parse_buffer(tcode_text_buffer_t *buffer, tcode_data_callback_t cb, char *response, size_t resplen);
tcode_err_t tcode_parse_stream(FILE* stream, tcode_text_buffer_t *buffer, tcode_data_callback_t cb, char *response, size_t resplen);

/** 
 * That would happen in some sort of main loop, if stream has valid data waiting to be read, this
 * will read it in. Should we perhaps have some sort of function to manage an internal buffer and
 * dispatch the relevant commands at the relevant time? Yeah alright that sounds good. So we need
 * a proper callback function, which we'll have here:
 */

typedef void (tcode_command_callback_t)(tcode_command_t);

/** 
 * And now to execute that buffer and invoke your callback. "But what to call it?" they said,
 * swigging a hearty mouthful of cider and staring into the abyss...
 * 
 *  static void callback(tcode_command_t command) {
 *      // The command struct you get here is going to be missing the increment and speed data.
 *      printf("I'm doing a %d command at %f magnitude now\n", command.type, command.magnitude);
 *  }
 */

tcode_err_t tcode_run_buffer(tcode_command_callback_t cmd, long ms);

/** 
 * Should the buffer be managed and passed in externally? How then would we manage the size and know
 * what commands are still processing, and how much time is left in the command? We could extend the
 * +tcode_command+ struct to include information about this, which would be handy when doing speed
 * or incremental based movements. Why both? Why not just support one and let the developer do the
 * calculation, like the feed rate flag in G-code.
 * 
 * I need more cider.
 * 
 * Alright, so since getting the current uTime is OS-dependent, and I don't want to shove free-RTOS
 * code into this because somebody is gonna copy and paste it and wonder why they can't get their
 * CP/M machine to respond to dick sucking commands, we have to pass in the current timestamp.
 * If there's something to do, it'll dispatch the command to your callback. If not, well, whatev.
 * 
 * Okay I don't like the idea of an external buffer. We'll manage the buffer internally but add a flag
 * to permit dynamic allocation or static allocation, in such a case we have either:
 * 
 *  tcode_command_t tcode_buffer[TCODE_BUFFER_LEN];
 * 
 * Or, a dynamic solution:
 * 
 *  typedef tcode_buffer_node {
 *      tcode_command_t command;
 *      tcode_buffer_node_t *next;
 *      tcode_buffer_note_t *prev;
 *  } tcode_buffer_node_t;
 * 
 *  typedef tcode_buffer {
 *      tcode_buffer_node *first;
 *      tcode_buffer_node *last;
 *  } tcode_buffer_t;
 * 
 * Yeah that's a doubly linked list shoutout to your COM104 class right? Anyway, since we got a linked
 * list up in here we've got to make a bunch of functions to manage that. But that's a problem for a 
 * different module, tcode_buffer.h.
 *  
 * And assuming we want to use the internal buffer with dynamic memory allocation and have the easiest
 * interface to tcode because honestly we just wanna have sex with a computer, right? So let's just make
 * this whole interface and expand as needed:
 * 
 * @example A Minimum Application
 *  void tcode_callback(tcode_command_t cmd);
 *  void data_callback(tcode_data_command_type_t cmd, FILE* stream);
 * 
 *  int app_main() {
 *      for(;;) {
 *          // tcode should parse multiple streams, but for commands that need output we need a way of
 *          // storing a stream handle to respond to, which would immediately get passed to a callback.
 *          // a different callback. a better callback. So we have a data and event callback.
 *          tcode_parse_stream(stdin, stdout, &data_callback);
 * 
 *          // This next bit will execute the events, taking in account any ramping that needs to happen.
 *          // All the relevant tweening is calculated and stored onto the internal buffer, so we need
 *          // only worry about handling whatever events tcode tells us to.
 *          tcode_run_buffer(&tcode_callback);
 *      }
 *  }
 * 
 * When a data command comes in, the data callback is invoked immediately. When any other commands come
 * in, they're parsed and added to the internal buffer. As soon as the parser gets a newline character,
 * the internal buffer will be flagged for execution, and any relevant events will be dispatched to the
 * callback. If there's nothing to do, this function just returns. Obviously.
 * 
 * This has been a train-of-thought header file* that will absolutely not parse into proper documentation.
 * 
 * Thanks!
 */

#ifdef __cplusplus
}
#endif

#endif