/**
*     _  _         _____                     _______  _ 
*    ( )( (    /| / ___ \ |\     /||\     /|(  ____ \( )
*    | ||  \  ( |( (   ) )( \   / )| )   ( || (    \/| |
*    (_)|   \ | |( (___) | \ (_) / | |   | || (_____ (_)
*     _ | (\ \) | \____  |  ) _ (  | |   | |(_____  ) _ 
*    ( )| | \   |      ) | / ( ) \ | |   | |      ) |( )
*    | || )  \  |/\____) )( /   \ )| (___) |/\____) || |
*    (_)|/    )_)\______/ |/     \|(_______)\_______)(_)
*                                                       
*/
/*  
*  Nyxus Source-Available Non-Derivative License
*  Copyright (c) 2026 Yazdan Samari
*  Permission is hereby granted, free of charge, to any person obtaining a copy
*  of this software and associated documentation files (the "Software"), to use
*  and compile the Software for personal or internal purposes, subject to the 
*  following conditions:
*  1. NO MODIFICATION: You may not modify, alter, translate, or create derivative 
*     works of the Software.
*  2. NO REDISTRIBUTION OF MODIFIED COPIES: You may not publish, distribute, 
*     sublicense, or sell modified versions of the Software.
*  3. ATTRIBUTION: The above copyright notice and this permission notice shall be 
*     included in all copies or substantial portions of the Software.
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
*  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
*  SOFTWARE.
*/

#pragma once

#ifndef _NYXUS_SOCKET_HPP_
#define _NYXUS_SOCKET_HPP_

#include <array>
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <freertos/task.h>
#include <lwip/sockets.h>
#include <lwip/netdb.h>
#include <Nyxus/nyx_network_types.hpp>
#include <Nyxus/nyx_terminal_graphics.hpp>

// MACRO UNDEFS BEGIN
#undef accept
#undef bind
#undef shutdown
#undef getpeername
#undef getsockname
#undef setsockopt
#undef getsockopt
#undef closesocket
#undef connect
#undef listen
#undef recv
#undef recvmsg
#undef recvfrom
#undef send
#undef sendmsg
#undef sendto
#undef socket
#undef select
#undef poll
#undef ioctlsocket
#undef read
#undef readv
#undef write
#undef writev
#undef close
#undef fcntl
#undef ioctl
// MACRO UNDEFS END

using domain_t   = int32_t;
using type_t     = int32_t;
using protocol_t = int32_t;
using socket_t   = int32_t;
using sockerr_t  = int32_t;
using shutdown_t = int32_t;
using flag_t     = int32_t;

/**
* @namespace Domain
* @brief Contains constants for socket domains (address families)
*/
namespace Domain {
	inline constexpr domain_t IPV4        = 0x02;   /**< Standard IP4 xxx.xxx.xxx                             */
	inline constexpr domain_t IPV6        = 0x0a;   /**< Standard IP6 xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:xxxx */
	inline constexpr domain_t UNSPECIFIED = 0x00;   /**< Uspecified                                           */
}

/**
* @namespace Type
* @brief Contains constants for socket types
*/
namespace Type {
	inline constexpr type_t TCP = 0x01;    /**< Transmission Control Protocol*/
	inline constexpr type_t UDP = 0x02;    /**< User Datagram Protocol       */
	inline constexpr type_t RAW = 0x03;    /**< No Protocol Specified        */
}

/**
* @namespace Protocol
* @brief Contains constants for socket protocols
*/
namespace Protocol {
	inline constexpr protocol_t IP      = 0x00;   /**< generic/auto -- placeholder for "let the stack pick" or setsockopt level               */
	inline constexpr protocol_t ICMP    = 0x01;   /**< ICMP -- raw sockets for ping traceroute network diagnostics                            */
	inline constexpr protocol_t TCP     = 0x06;   /**< TCP -- reliable stream transport auto-selected for SOCK_STREAM                         */
	inline constexpr protocol_t UDP     = 0x11;   /**< UDP -- connectionless datagram transport auto-selected for SOCK_DGRAM                  */
	inline constexpr protocol_t IPV6    = 0x29;   /**< IPv6 -- used when tunneling IPv6 packets inside IPv4 (6in4)                            */
	inline constexpr protocol_t ICMPV6  = 0x3A;   /**< ICMPv6 -- raw sockets for IPv6 diagnostics neighbor discovery router solicitation      */
	inline constexpr protocol_t UDPLITE = 0x88;   /**< UDP-Lite -- datagram transport with partial checksum coverage rare multimedia/VoIP use */
	inline constexpr protocol_t RAW     = 0xFF;   /**< raw IP -- full self-built packets including header for custom crafting/spoofing        */
}

/**
* @namespace ShutDownOption
* @brief Contains constants for socket shutdown options
*/
namespace ShutDownOption {
	inline constexpr shutdown_t RCV = 0x00;   /**< disable further receives -- stop reading. peer can still send but you'll ignore/discard it 	 */
	inline constexpr shutdown_t SND = 0x01;   /**< disable further sends -- sends a TCP FIN. tells peer "I'm done writing" but you can still read */
	inline constexpr shutdown_t ALL = 0x02;   /**< disable both directions -- full shutdown of I/O though fd itself is still open until close()   */
}

/**
* @namespace MessageFlag
* @brief Contains constants for socket message flags
*/
namespace MessageFlag {
	inline constexpr flag_t NONE     = 0x00;  /**< No flags specified             */
	inline constexpr flag_t PEEK     = 0x01;  /**< Peek at incoming messages      */
	inline constexpr flag_t WAITALL  = 0x02;  /**< Wait for full request or error */
	inline constexpr flag_t OOB      = 0x04;  /**< Process out-of-band data       */
	inline constexpr flag_t DONTWAIT = 0x08;  /**< Non-blocking operation         */
	inline constexpr flag_t MORE     = 0x10;  /**< Sender will send more          */
}

/**
* @struct address_t
* @brief Represents a socket address, encapsulating sockaddr_storage and its length.
*/
struct address_t {
	public:
	/**
	* @brief The underlying socket address storage
	*/
	sockaddr_storage storage{};
	
	public:
	/**
	 * @brief The length of the socket address
	 */
	socklen_t len = sizeof(sockaddr_storage);
	
	public:
	/**
	 * @brief Get a pointer to the raw socket address
	 * @return A pointer to the raw socket address
	 */
	sockaddr* raw() {
		return reinterpret_cast<sockaddr*>(&storage);
	}
	
	public:
	/**
	 * @brief Get a pointer to the raw socket address
	 * @return A pointer to the raw socket address
	 */
	const sockaddr* raw() const {
		return reinterpret_cast<const sockaddr*>(&storage);
	}
};

class NYXUS_SOCKET {
	protected:
	/**
	* @brief The address associated with the socket
	*/
	address_t address;

	protected:
	/**
	* @brief The domain associated with the socket
	*/
	domain_t Domain;

	protected:
	/**
	* @brief The type associated with the socket
	*/
	type_t Type;

	protected:
	/**
	* @brief The protocol associated with the socket
	*/
	protocol_t Protocol;

	protected:
	/**
	* @brief The file descriptor associated with the socket
	*/
	socket_t Socket;

	private:
	/** 
	* @brief Display error message for a given socket error
	* @paragraph This function displays an error message for a given socket error.
	* @param se The socket error
	* @return The error message
	*/
	static inline const char * __restrict displayErr(sockerr_t se);

	public:
	// Default Constructor
	NYXUS_SOCKET() : address(), Domain(Domain::UNSPECIFIED), Type(0), Protocol(0), Socket(-1) {};

	public:
	// Constructor for explicitly wrapping an existing FD (e.g. from accept)
	NYXUS_SOCKET(socket_t fd, domain_t d, type_t t, protocol_t p, const address_t& addr) : address(addr), Domain(d), Type(t), Protocol(p), Socket(fd) {};
	
	public:
	// Constructor mapping to standard socket() invocation
	NYXUS_SOCKET(domain_t d, type_t t, protocol_t p, bool verbose = false) : address(), Domain(d), Type(t), Protocol(p), Socket(-1) {
		if ((Socket = static_cast<socket_t>(lwip_socket(Domain, Type, Protocol))) < 0) {
			if (verbose) {
				Serial.printf("[%sERROR%s] Failed To create Socket, reason: %s", Color::RED, Color::RESET, displayErr(errno));
			}
		}
	}

	public:
	// Destructor - Closes socket to prevent resource leaks
	~NYXUS_SOCKET() {
		if (Socket >= 0) {
			lwip_close(Socket);
			Socket = -1;
		}
	}

	public:
	// Move Constructor - Transfers ownership of the socket FD
	NYXUS_SOCKET(NYXUS_SOCKET&& other) noexcept
		: address(other.address), Domain(other.Domain), Type(other.Type), Protocol(other.Protocol), Socket(other.Socket) {
		other.Socket = -1; // Nullify source to prevent double close
	}

	public:
	NYXUS_SOCKET& operator=(NYXUS_SOCKET&& other) noexcept {
		if (this != &other) {
			if (Socket >= 0) lwip_close(Socket);
			address = other.address;
			Domain = other.Domain;
			Type = other.Type;
			Protocol = other.Protocol;
			Socket = other.Socket;
			other.Socket = -1;
		}
		return *this;
	}

	public:
	NYXUS_SOCKET(const NYXUS_SOCKET&) = delete;
	
	public:
	NYXUS_SOCKET& operator=(const NYXUS_SOCKET&) = delete;

	public:
	/** 
	* @brief Get the file descriptor for the socket
	* @paragraph This function returns the underlying file descriptor associated with the socket. It is useful for low-level operations or when interfacing with APIs that require a raw socket descriptor.
	* @return The file descriptor
	*/
	[[nodiscard]] inline socket_t descriptor() const {
		return Socket;
	}

	public:
	/** 
	* @brief Accept a new connection on the socket
	* @paragraph This function accepts a new connection on the socket and returns a new NYXUS_SOCKET representing the accepted connection.
	* @return A new NYXUS_SOCKET representing the accepted connection
	*/
	[[nodiscard]] inline NYXUS_SOCKET accept() const {
		address_t peer_addr;
		socket_t new_fd = static_cast<socket_t>(lwip_accept(Socket, peer_addr.raw(), &peer_addr.len));
		return NYXUS_SOCKET(new_fd, Domain, Type, Protocol, peer_addr);
	}

	public:
	/** 
	* @brief Bind the socket to a local address
	* @paragraph This function binds the socket to a local address.
	* @param local The local address to bind to
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int bind(const NYXUS_SOCKET& local) {
		address = local.address;
		return lwip_bind(Socket, address.raw(), address.len);
	}

	public:
	/** 
	* @brief Connect the socket to a remote address
	* @paragraph This function connects the socket to a remote address.
	* @param remote The remote address to connect to
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int connect(const NYXUS_SOCKET& remote) {
		address = remote.address;
		return lwip_connect(Socket, address.raw(), address.len);
	}

	public:
	/** 
	* @brief Connect the socket to a remote host and port
	* @paragraph This function resolves the host domain and connects the socket.
	* @param host The remote hostname to connect to
	* @param port The remote port to connect to
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int connect(const char* host, uint16_t port) {
		struct addrinfo hints = {};
		hints.ai_family = Domain;
		hints.ai_socktype = Type;
		hints.ai_protocol = Protocol;
		struct addrinfo *res;
		char port_str[6];
		snprintf(port_str, sizeof(port_str), "%u", port);
		int err = getaddrinfo(host, port_str, &hints, &res);
		if (err != 0 || res == nullptr) return -1;
		memcpy(&address.storage, res->ai_addr, res->ai_addrlen);
		address.len = res->ai_addrlen;
		int ret = lwip_connect(Socket, address.raw(), address.len);
		freeaddrinfo(res);
		return ret;
	}

	public:
	/** 
	* @brief Listen for connections on the socket
	* @paragraph This function puts the socket in a state where it can accept incoming connections.
	* @param backlog The maximum number of pending connections
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int listen(int backlog) {
		return lwip_listen(Socket, backlog);
	}

	public:
	/** 
	* @brief Shutdown the socket
	* @paragraph This function shuts down the socket.
	* @param option The shutdown option
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int shutdown(shutdown_t option) {
		return lwip_shutdown(Socket, option);
	}

	public:
	/** 
	* @brief Close the socket
	* @paragraph This function closes the socket.
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int close() {
		int result = lwip_close(Socket);
		Socket = -1;
		return result;
	}

	public:
	/** 
	* @brief Get the name of the peer socket
	* @paragraph This function gets the name of the peer socket.
	* @param peer The peer socket
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int getpeername(NYXUS_SOCKET& peer) const {
		return lwip_getpeername(Socket, peer.address.raw(), &peer.address.len);
	}

	public:
	/** 
	* @brief Get the name of the local socket
	* @paragraph This function gets the name of the local socket.
	* @param local The local socket
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int getsockname(NYXUS_SOCKET& local) const {
		return lwip_getsockname(Socket, local.address.raw(), &local.address.len);
	}

	public:
	/** 
	* @brief Set socket options
	* @paragraph This function sets socket options.
	* @param level The protocol level
	* @param optname The option name
	* @param value The option value
	* @return 0 on success, -1 on failure
	*/
	template <typename T>
	[[nodiscard]] inline int setsockopt(int level, int optname, const T& value) {
		return lwip_setsockopt(Socket, level, optname, &value, sizeof(T));
	}

	public:
	/** 
	* @brief Get socket options
	* @paragraph This function gets socket options.
	* @param level The protocol level
	* @param optname The option name
	* @param value The option value
	* @return 0 on success, -1 on failure
	*/
	template <typename T>
	[[nodiscard]] inline int getsockopt(int level, int optname, T& value) const {
		socklen_t len = sizeof(T);
		return lwip_getsockopt(Socket, level, optname, &value, &len);
	}

	public:
	/** 
	* @brief Send data on the socket
	* @paragraph This function sends data on the socket.
	* @param data The data to send
	* @param size The size of the data
	* @param flags The send flags
	* @return The number of bytes sent on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t send(const void* data, size_t size, flag_t flags = MessageFlag::NONE) {
		return lwip_send(Socket, data, size, flags);
	}

	public:
	/** 
	* @brief Send data to a specific address
	* @paragraph This function sends data to a specific address.
	* @param data The data to send
	* @param size The size of the data
	* @param to The destination socket
	* @param flags The send flags
	* @return The number of bytes sent on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t sendto(const void* data, size_t size, const NYXUS_SOCKET& to, flag_t flags = MessageFlag::NONE) {
		return lwip_sendto(Socket, data, size, flags, to.address.raw(), to.address.len);
	}

	public:
	/** 
	* @brief Send a message on the socket
	* @paragraph This function sends a message on the socket.
	* @param message The message to send
	* @param flags The send flags
	* @return The number of bytes sent on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t sendmsg(const msghdr* message, flag_t flags = MessageFlag::NONE) {
		return lwip_sendmsg(Socket, message, flags);
	}

	public:
	/** 
	* @brief Receive data on the socket
	* @paragraph This function receives data on the socket.
	* @param buffer The buffer to receive data into
	* @param len The size of the buffer
	* @param flags The receive flags
	* @return The number of bytes received on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t recv(void* buffer, size_t len, flag_t flags = MessageFlag::NONE) {
		return lwip_recv(Socket, buffer, len, flags);
	}

	public:
	/** 
	* @brief Receive data from a specific address
	* @paragraph This function receives data from a specific address.
	* @param buffer The buffer to receive data into
	* @param len The size of the buffer
	* @param from The source socket
	* @param flags The receive flags
	* @return The number of bytes received on success, -1 on failure
	*/	
	[[nodiscard]] inline ssize_t recvfrom(void* buffer, size_t len, NYXUS_SOCKET& from, flag_t flags = MessageFlag::NONE) {
		return lwip_recvfrom(Socket, buffer, len, flags, from.address.raw(), &from.address.len);
	}

	public:
	/** 
	* @brief Receive a message on the socket
	* @paragraph This function receives a message on the socket.
	* @param message The message to receive
	* @param flags The receive flags
	* @return The number of bytes received on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t recvmsg(msghdr* message, flag_t flags = MessageFlag::NONE) {
		return lwip_recvmsg(Socket, message, flags);
	}

	public:
	/** 
	* @brief Read data from the socket
	* @paragraph This function reads data from the socket.
	* @param buffer The buffer to read data into
	* @param len The size of the buffer
	* @return The number of bytes read on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t read(void* buffer, size_t len) {
		return lwip_read(Socket, buffer, len);
	}

	public:
	/** 
	* @brief Write data to the socket
	* @paragraph This function writes data to the socket.
	* @param data The data to write
	* @param len The size of the data
	* @return The number of bytes written on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t write(const void* data, size_t len) {
		return lwip_write(Socket, data, len);
	}

	public:
	/** 
	* @brief Read data from the socket using scatter-gather I/O
	* @paragraph This function reads data from the socket using scatter-gather I/O.
	* @param iov The array of iovec structures
	* @param iovcnt The number of iovec structures
	* @return The number of bytes read on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t readv(const iovec* iov, int iovcnt) {
		return lwip_readv(Socket, iov, iovcnt);
	}

	public:
	/** 
	* @brief Write data to the socket using scatter-gather I/O
	* @paragraph This function writes data to the socket using scatter-gather I/O.
	* @param iov The array of iovec structures
	* @param iovcnt The number of iovec structures
	* @return The number of bytes written on success, -1 on failure
	*/
	[[nodiscard]] inline ssize_t writev(const iovec* iov, int iovcnt) {
		return lwip_writev(Socket, iov, iovcnt);
	}

	public:
	/** 
	* @brief Perform I/O control operations on the socket
	* @paragraph This function performs I/O control operations on the socket.
	* @param cmd The control command
	* @param argp The argument for the control command
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int fcntl(int cmd, int val) {
		return lwip_fcntl(Socket, cmd, val);
	}

	public:
	/** 
	* @brief Perform I/O control operations on the socket
	* @paragraph This function performs I/O control operations on the socket.
	* @param cmd The control command
	* @param argp The argument for the control command
	* @return 0 on success, -1 on failure
	*/
	[[nodiscard]] inline int ioctl(long cmd, void* argp) {
		return lwip_ioctl(Socket, cmd, argp);
	}

	#if LWIP_SOCKET_SELECT
	public:
	/** 
	* @brief Perform select operation on the socket
	* @paragraph This function performs select operation on the socket.
	* @param maxfdp1 The maximum file descriptor plus one
	* @param readset The set of file descriptors to check for reading
	* @param writeset The set of file descriptors to check for writing
	* @param exceptset The set of file descriptors to check for exceptions
	* @param timeout The timeout value
	* @return The number of file descriptors ready on success, -1 on failure
	*/
	[[nodiscard]] static inline int select(int maxfdp1, fd_set* readset, fd_set* writeset, fd_set* exceptset, timeval* timeout) {
		return lwip_select(maxfdp1, readset, writeset, exceptset, timeout);
	}
	#endif

	#if LWIP_SOCKET_POLL
	public:
	/**
	* @brief Perform poll operation on the socket
	* @paragraph This function performs poll operation on the socket.
	* @param fds The array of pollfd structures
	* @param nfds The number of pollfd structures
	* @param timeout The timeout value
	* @return The number of file descriptors ready on success, -1 on failure
	*/
	[[nodiscard]] static inline int poll(pollfd* fds, nfds_t nfds, int timeout) {
		return lwip_poll(fds, nfds, timeout);
	}
	#endif
};

// IMPLEMENTATIONS - NOTHING HERE

inline const char* __restrict NEXUS_SOCKET::displayErr(sockerr_t se) {
	switch (se) {
		case 1:   return "Not owner"; break;
		case 2:   return "No such file or directory"; break;
		case 3:   return "No such process"; break;
		case 4:   return "Interrupted system call"; break;
		case 5:   return "I/O error"; break;
		case 6:   return "No such device or address"; break;
		case 7:   return "Arg list too long"; break;
		case 8:   return "Exec format error"; break;
		case 9:   return "Bad file number"; break;
		case 10:  return "No children"; break;
		case 11:  return "No more processes"; break;
		case 12:  return "Not enough space"; break;
		case 13:  return "Permission denied"; break;
		case 14:  return "Bad address"; break;
		case 15:  return "Block device required"; break;
		case 16:  return "Device or resource busy"; break;
		case 17:  return "File exists"; break;
		case 18:  return "Cross-device link"; break;
		case 19:  return "No such device"; break;
		case 20:  return "Not a directory"; break;
		case 21:  return "Is a directory"; break;
		case 22:  return "Invalid argument"; break;
		case 23:  return "Too many open files in system"; break;
		case 24:  return "File descriptor value too large"; break;
		case 25:  return "Not a character device"; break;
		case 26:  return "Text file busy"; break;
		case 27:  return "File too large"; break;
		case 28:  return "No space left on device"; break;
		case 29:  return "Illegal seek"; break;
		case 30:  return "Read-only file system"; break;
		case 31:  return "Too many links"; break;
		case 32:  return "Broken pipe"; break;
		case 33:  return "Mathematics argument out of domain of function"; break;
		case 34:  return "Result too large"; break;
		case 35:  return "No message of desired type"; break;
		case 36:  return "Identifier removed"; break;
		case 37:  return "Channel number out of range"; break;
		case 38:  return "Level 2 not synchronized"; break;
		case 39:  return "Level 3 halted"; break;
		case 40:  return "Level 3 reset"; break;
		case 41:  return "Link number out of range"; break;
		case 42:  return "Protocol driver not attached"; break;
		case 43:  return "No CSI structure available"; break;
		case 44:  return "Level 2 halted"; break;
		case 45:  return "Deadlock"; break;
		case 46:  return "No lock"; break;
		case 50:  return "Invalid exchange"; break;
		case 51:  return "Invalid request descriptor"; break;
		case 52:  return "Exchange full"; break;
		case 53:  return "No anode"; break;
		case 54:  return "Invalid request code"; break;
		case 55:  return "Invalid slot"; break;
		case 56:  return "File locking deadlock error"; break;
		case 57:  return "Bad font file fmt"; break;
		case 60:  return "Not a stream"; break;
		case 61:  return "No data (for no delay io)"; break;
		case 62:  return "Stream ioctl timeout"; break;
		case 63:  return "No stream resources"; break;
		case 64:  return "Machine is not on the network"; break;
		case 65:  return "Package not installed"; break;
		case 66:  return "The object is remote"; break;
		case 67:  return "Virtual circuit is gone"; break;
		case 68:  return "Advertise error"; break;
		case 69:  return "Srmount error"; break;
		case 70:  return "Communication error on send"; break;
		case 71:  return "Protocol error"; break;
		case 74:  return "Multihop attempted"; break;
		case 75:  return "Inode is remote (not really error)"; break;
		case 76:  return "Cross mount point (not really error)"; break;
		case 77:  return "Bad message"; break;
		case 79:  return "Inappropriate file type or format"; break;
		case 80:  return "Given log. name not unique"; break;
		case 81:  return "f.d. invalid for this operation"; break;
		case 82:  return "Remote address changed"; break;
		case 83:  return "Can't access a needed shared lib"; break;
		case 84:  return "Accessing a corrupted shared lib"; break;
		case 85:  return ".lib section in a.out corrupted"; break;
		case 86:  return "Attempting to link in too many libs"; break;
		case 87:  return "Attempting to exec a shared library"; break;
		case 88:  return "Function not implemented"; break;
		case 89:  return "No more files"; break;
		case 90:  return "Directory not empty"; break;
		case 91:  return "File or path name too long"; break;
		case 92:  return "Too many symbolic links"; break;
		case 95:  return "Operation not supported on socket"; break;
		case 96:  return "Protocol family not supported"; break;
		case 104: return "Connection reset by peer"; break;
		case 105: return "No buffer space available"; break;
		case 106: return "Address family not supported by protocol family"; break;
		case 107: return "Protocol wrong type for socket"; break;
		case 108: return "Socket operation on non-socket"; break;
		case 109: return "Protocol not available"; break;
		case 110: return "Can't send after socket shutdown"; break;
		case 111: return "Connection refused"; break;
		case 112: return "Address already in use"; break;
		case 113: return "Software caused connection abort"; break;
		case 114: return "Network is unreachable"; break;
		case 115: return "Network interface is not configured"; break;
		case 116: return "Connection timed out"; break;
		case 117: return "Host is down"; break;
		case 118: return "Host is unreachable"; break;
		case 119: return "Connection already in progress"; break;
		case 120: return "Socket already connected"; break;
		case 121: return "Destination address required"; break;
		case 122: return "Message too long"; break;
		case 123: return "Unknown protocol"; break;
		case 124: return "Socket type not supported"; break;
		case 125: return "Address not available"; break;
		case 126: return "Connection aborted by network"; break;
		case 127: return "Socket is already connected"; break;
		case 128: return "Socket is not connected"; break;
		case 135: return "No medium (in tape drive)"; break;
		case 136: return "No such host or network path"; break;
		case 137: return "Filename exists with different case"; break;
		case 138: return "Illegal byte sequence"; break;
		case 139: return "Value too large for defined data type"; break;
		case 140: return "Operation canceled"; break;
		case 141: return "State not recoverable"; break;
		case 142: return "Previous owner died"; break;
		case 143: return "Streams pipe error"; break;
		default:  return "Unknown Error"; break;
	}
}

#endif