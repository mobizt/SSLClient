/* Copyright 2019 OSU OPEnS Lab
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "bearssl.h"
#include "Arduino.h"
#include "Client.h"
#include "SSLSession.h"

#ifndef SSLClientImpl_H_
#define SSLClientImpl_H_

/**
 * @brief Static constants defining the possible errors encountered.
 * 
 * If SSLClient encounters an error, it will generally output
 * logs into the serial moniter. If you need a way of programmatically
 * checking the errors, you can do so with SSLClient::getWriteError(),
 * which will return one of these values.
 */
enum Error {
    SSL_OK = 0,
    /** The underlying client failed to connect, probably not an issue with SSL */
    SSL_CLIENT_CONNECT_FAIL,
    /** BearSSL failed to complete the SSL handshake, check logs for bear ssl error output */
    SSL_BR_CONNECT_FAIL,
    /** The underlying client failed to write a payload, probably not an issue with SSL */
    SSL_CLIENT_WRTIE_ERROR,
    /** An internal error occurred with BearSSL, check logs for diagnosis. */
    SSL_BR_WRITE_ERROR,
    /** An internal error occured with SSLClient, and you probably need to submit an issue on Github. */
    SSL_INTERNAL_ERROR,
    /** SSLClient detected that there was not enough memory (>8000 bytes) to continue. */
    SSL_OUT_OF_MEMORY
};

/**
 * @brief Level of verbosity used in logging for SSLClient.
 * 
 * Use these values when initializing SSLCLient to set how many logs you
 * would like to see in the Serial moniter.
 */
enum DebugLevel {
    /** No logging output */
    SSL_NONE = 0,
    /** Only output errors that result in connection failure */
    SSL_ERROR = 1,
    /** Ouput errors and warnings (useful when just starting to develop) */
    SSL_WARN = 2,
    /** Output errors, warnings, and internal information (very verbose) */
    SSL_INFO = 3,
};

/** 
 * On error, any function in this class will terminate the socket.
 * TODO: Write what this is */

class SSLClientImpl: public Client {
public:
    /**
     * @brief initializes SSL contexts for bearSSL
     * 
     * @pre You will need to generate an array of trust_anchors (root certificates)
     * based off of the domains you want to make SSL connections to. Check out the
     * Wiki on the pycert-bearssl tool for a simple way to do this.
     * @pre The analog_pin should be set to input.
     * 
     * @post set_client must be called immediatly after to set the client class
     * pointer.
     * 
     * @param trust_anchors Trust anchors used in the verification 
     * of the SSL server certificate, generated using the `brssl` command
     * line utility. For more information see the samples or bearssl.org
     * @param trust_anchors_num The number of trust anchors stored
     * @param analog_pin An analog pin to pull random bytes from, used in seeding the RNG
     * @param get_remote_ip Function pointer to get the remote ip from the client. We
     * need this value since the Client abstract class has no remoteIP() function,
     * however most of the arduino internet client implementations do.
     * @param debug whether to enable or disable debug logging, must be constexpr
     */
    explicit SSLClientImpl(Client* client, const br_x509_trust_anchor *trust_anchors, 
        const size_t trust_anchors_num, const int analog_pin, const DebugLevel debug);

    //============================================
    //= Functions implemented in SSLClientImpl.cpp
    //============================================

    /** @see SSLClient::connect(IPAddress, uint16_t) */
    int connect_impl(IPAddress ip, uint16_t port);
    /** @see SSLClient::connect(const char*, uint16_t) */
	int connect_impl(const char *host, uint16_t port);
    /** @see SSLClient::write(const uint8_t*, size_t) */
	size_t write_impl(const uint8_t *buf, size_t size);
    /** @see SSLClient::available */
	int available_impl();
    /** @see SSLClient::read(uint8_t*, size_t) */
	int read_impl(uint8_t *buf, size_t size);
    /** @see SSLClient::peek */
	int peek_impl();
    /** @see SSLClient::flush */
	void flush_impl();
    /** @see SSLClient::stop */
	void stop_impl();
    /** @see SSLClient::connected */
	uint8_t connected_impl();

    //============================================
    //= Functions implemented in SSLClient.h
    //============================================
    /** See SSLClient::localPort */
    virtual uint16_t localPort() = 0;
    /** See SSLClient::remoteIP */
	virtual IPAddress remoteIP() = 0;
    /** See SSLClient::localPort */
	virtual uint16_t remotePort() = 0;
    /** See SSLClient::getSession */
    virtual SSLSession& getSession(const char* host, const IPAddress& addr) = 0;
    
protected:

    //============================================
    //= Functions implemented in SSLClientImpl.cpp
    //============================================

    /** 
     * @brief set the pointer to the Client class that we wil use
     * 
     * Call this function immediatly after the ctor. This functionality
     * is placed in it's own function for flexibility reasons, but it
     * is critical that this function is called before anything else
     */
    void set_client(Client* c) { m_client = c; }

    /** @brief Prints a debugging prefix to all logs, so we can attatch them to useful information */
    void m_print_prefix(const char* func_name, const DebugLevel level) const;

    /** @brief Prints the string associated with a write error */
    void m_print_ssl_error(const int ssl_error, const DebugLevel level) const;

    /** @brief Print the text string associated with a BearSSL error code */
    void m_print_br_error(const unsigned br_error_code, const DebugLevel level) const;

    /** @brief debugging print function, only prints if m_debug is true */
    template<typename T>
    void m_print(const T str, const char* func_name, const DebugLevel level) const { 
        // check the current debug level and serial status
        if (level > m_debug || !Serial) return;
        // print prefix
        m_print_prefix(func_name, level);
        // print the message
        Serial.println(str);
    }

    /** @brief Prints a info message to serial, if info messages are enabled */
    template<typename T>
    void m_info(const T str, const char* func_name) const { m_print(str, func_name, SSL_INFO); }

    template<typename T>
    void m_warn(const T str, const char* func_name) const { m_print(str, func_name, SSL_WARN); }

    template<typename T>
    void m_error(const T str, const char* func_name) const { m_print(str, func_name, SSL_ERROR); }

private:
    /** Returns whether or not the engine is connected, without polling the client over SPI or other (as opposed to connected()) */
    bool m_soft_connected(const char* func_name);
    /** start the ssl engine on the connected client */
    int m_start_ssl(const char* host, SSLSession& ssl_ses);
    /** run the bearssl engine until a certain state */
    int m_run_until(const unsigned target);
    /** proxy for availble that returns the state */
    unsigned m_update_engine(); 
    
    //============================================
    //= Data Members
    //============================================
    
    // hold a reference to the client
    Client* m_client;
    // store pointers to the trust anchors
    // should not be computed at runtime
    const br_x509_trust_anchor *m_trust_anchors;
    const size_t m_trust_anchors_num;
    // store the pin to fetch an RNG see from
    const int m_analog_pin;
    // store whether to enable debug logging
    const DebugLevel m_debug;
    // store the context values required for SSL
    br_ssl_client_context m_sslctx;
    br_x509_minimal_context m_x509ctx;
    // use a mono-directional buffer by default to cut memory in half
    // can expand to a bi-directional buffer with maximum of BR_SSL_BUFSIZE_BIDI
    // or shrink to below BR_SSL_BUFSIZE_MONO, and bearSSL will adapt automatically
    // simply edit this value to change the buffer size to the desired value
    // additionally, we need to correct buffer size based off of how many sessions we decide to cache
    // since SSL takes so much memory if we don't it will cause the stack and heap to collide 
    /**
     * @brief The internal buffer to use with BearSSL.
     * This buffer controls how much data BearSSL can encrypt/decrypt at a given time. It can be expanded
     * or shrunk to [255, BR_SSL_BUFSIZE_BIDI], depending on the memory and speed needs of your application.
     * As a rule of thumb SSLClient will fail if it does not have at least 8000 bytes when starting a
     * connection.
     */
    unsigned char m_iobuf[BR_SSL_BUFSIZE_MONO / 4];
    static_assert(sizeof m_iobuf <= BR_SSL_BUFSIZE_BIDI, "m_iobuf must be below maximum buffer size");
    // store the index of where we are writing in the buffer
    // so we can send our records all at once to prevent
    // weird timing issues
    size_t m_write_idx;
};

#endif /* SSLClientImpl_H_ */