/* The GPL applies to this program.
  In addition, as a special exception, the copyright holders give
  permission to link the code of portions of this program with the
  OpenSSL library under certain conditions as described in each
  individual source file, and distribute linked combinations
  including the two.
  You must obey the GNU General Public License in all respects
  for all of the code used other than OpenSSL.  If you modify
  file(s) with this exception, you may extend this exception to your
  version of the file(s), but you are not obligated to do so.  If you
  do not wish to do so, delete this exception statement from your
  version.  If you delete this exception statement from all source
  files in the program, then also delete it here.
*/

#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/md5.h>

#include "mssl.h"

extern char last_error[];
BIO *bio_err=0;

char close_ssl_connection(SSL *ssl_h, int socket_h)
{
	int rc = SSL_shutdown(ssl_h);

	if (!rc)
	{
		shutdown(socket_h, 1);

		rc = SSL_shutdown(ssl_h);
	}

	/* rc == 0 means try again but it seems to be fine
	 * to ignore that is what I read from the manpage
	 */
	if (rc == -1)
		return -1;
	else 
		return 0;
}

int READ_SSL(SSL *ssl_h, char *whereto, int len)
{
	int cnt=len;

	while(len>0)
	{
		int rc;

		rc = SSL_read(ssl_h, whereto, len);
		if (rc == -1)
		{
			if (errno != EINTR && errno != EAGAIN)
			{
				sprintf(last_error, "READ_SSL: io-error: %s\n", strerror(errno));
				return -1;
			}
		}
		else if (rc == 0)
		{
			return 0;
		}
		else
		{
			whereto += rc;
			len -= rc;
		}
	}

	return cnt;
}

int WRITE_SSL(SSL *ssl_h, char *whereto, int len)
{
	int cnt=len;

	while(len>0)
	{
		int rc;

		rc = SSL_write(ssl_h, whereto, len);
		if (rc == -1)
		{
			if (errno != EINTR && errno != EAGAIN)
			{
				sprintf(last_error, "WRITE_SSL: io-error: %s\n", strerror(errno));
				return -1;
			}
		}
		else if (rc == 0)
		{
			return 0;
		}
		else
		{
			whereto += rc;
			len -= rc;
		}
	}

	return cnt;
}

int connect_ssl(int socket_h, SSL_CTX *client_ctx, SSL **ssl_h, BIO **s_bio, int timeout)
{
	int dummy;
#if 0
	int rc;
	struct timeval to;
	fd_set rfds;

	FD_ZERO(&rfds);
	FD_SET(socket_h, &rfds);

	to.tv_sec  = timeout / 1000;
	to.tv_usec = (timeout - (to.tv_sec * 1000)) * 1000;

	/* wait for connection */
	rc = select(socket_h + 1, &rfds, NULL, NULL, &to);
	if (rc == 0)
		return -2;      /* timeout */
	else if (rc == -1)
	{
		if (errno == EINTR)
			return -3;      /* ^C pressed */
		else
			return -1;      /* error */
	}
#endif

	*ssl_h = SSL_new(client_ctx);
	*s_bio = BIO_new_socket(socket_h, BIO_NOCLOSE);
	SSL_set_bio(*ssl_h, *s_bio, *s_bio);
	dummy = SSL_connect(*ssl_h);
	if (dummy <= 0)
	{
		sprintf(last_error, "problem starting SSL connection: %d\n", SSL_get_error(*ssl_h, dummy));

		return -1;
	}

	return 0;
}

SSL_CTX * initialize_ctx(void)
{
	SSL_METHOD *meth;

	if (!bio_err)
	{
		SSL_library_init();
		SSL_load_error_strings();

		/* error write context */
		bio_err = BIO_new_fp(stderr, BIO_NOCLOSE);
	}

	/* create context */
	meth = SSLv23_method();

	return SSL_CTX_new(meth);
}

char * get_fingerprint(SSL *ssl_h)
{
	char *string = NULL;

	unsigned char fp_digest[EVP_MAX_MD_SIZE];
	X509 *x509_data = SSL_get_peer_certificate(ssl_h);

	if (x509_data)
	{
		unsigned int fp_digest_size = sizeof(fp_digest);

		memset(fp_digest, 0x00, sizeof(fp_digest));

		if (X509_digest(x509_data, EVP_md5(), fp_digest, &fp_digest_size))
		{
			string = (char *)malloc(MD5_DIGEST_LENGTH * 3 + 1);
			if (string)
			{
				int loop, pos =0;

				for(loop=0; loop<MD5_DIGEST_LENGTH; loop++)
				{
					if (loop)
						pos += sprintf(&string[pos], ":%02x", fp_digest[loop]);
					else
						pos = sprintf(&string[pos], "%02x", fp_digest[loop]);
				}
			}
		}

		X509_free(x509_data);
	}

	return string;
}
