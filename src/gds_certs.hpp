#ifndef GDS_CERTS_HPP
#define GDS_CERTS_HPP

#include <cstdio>
#include <cstdlib>
#include <string>
#include <tuple>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/pkcs12.h>

std::pair <std::string,std::string> parse_cert(const std::string& path, const std::string& password)
{
    std::FILE *fp;
    EVP_PKEY *pkey = NULL;
    X509 *cert = NULL;
    STACK_OF(X509) *ca = NULL;
    
    if ((fp = fopen(path.c_str(), "rb")) == NULL) {
    	return {};
    }

    PKCS12 *p12 = d2i_PKCS12_fp(fp, NULL);

    fclose(fp);

    if (p12 == NULL) {
        return {};
    }

    if (!PKCS12_parse(p12, password.c_str(), &pkey, &cert, &ca)) {
	    X509_free(cert);
	    EVP_PKEY_free(pkey);
	    sk_X509_pop_free(ca, X509_free);
        return {};
    }
    PKCS12_free(p12);

    std::pair<std::string, std::string> files;
    files.first = std::tmpnam(nullptr);
    files.second = std::tmpnam(nullptr);

    std::FILE* cert_file = fopen(files.first.c_str(), "wb");
    PEM_write_X509_AUX(cert_file, cert);
    fclose(cert_file);

    std::FILE* private_key_file = fopen(files.second.c_str(), "wb");
    PEM_write_PrivateKey(private_key_file, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(private_key_file);

    return files;
}

#endif