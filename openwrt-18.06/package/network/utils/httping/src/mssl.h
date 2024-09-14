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

char close_ssl_connection(SSL *ssl_h, int socket_h);
int READ_SSL(SSL *ssl_h, char *whereto, int len);
int WRITE_SSL(SSL *ssl_h, char *whereto, int len);
int connect_ssl(int socket_h, SSL_CTX *client_ctx, SSL **ssl_h, BIO **s_bio, int timeout);
SSL_CTX * initialize_ctx(void);
char * get_fingerprint(SSL *ssl_h);
