#pragma once

#include <cstdlib>

#include <string>
#include <functional>
#include <memory>
#include <array>

namespace ssl_helpers {

namespace impl {
    class aes_encryption_stream_impl;
    class aes_decryption_stream_impl;
} // namespace impl

//Base types

using aes_256bit_type = std::array<char, 16>;

using aes_tag_type = aes_256bit_type;

using aes_salt_type = aes_256bit_type;
using salted_key_type = std::pair<std::string /*encryption key*/, aes_salt_type /*random salt*/>;

std::string aes_to_string(const aes_256bit_type&);
aes_256bit_type aes_from_string(const std::string&);

//Create tagged data stream that includes tag data of encrypted stream
//and optionally 'marker' is ADD (Additional Authenticated Data)
//Implementation guarantee authenticity of ADD and Data through the tag.
//  Stream:
//
// -> |ADD (can be readable)|encrypted data (bynary)|TAG (bynary 16 sz)|
//

class aes_encryption_stream
{
public:
    //these KEY and ADD will be stored for each encryption session if no one set in this->start
    aes_encryption_stream(const std::string& key = {}, const std::string& additional_authenticated_data = {});
    ~aes_encryption_stream();

    //start encryption session. Key is required here or in constructor
    std::string start(const std::string& key = {}, const std::string& marker = {});
    std::string encrypt(const std::string& plain_chunk);
    //finalize encryption session and create tag
    aes_tag_type finalize();

private:
    std::unique_ptr<impl::aes_encryption_stream_impl> _impl;
};

class aes_decryption_stream
{
public:
    //these KEY and ADD will be stored for each decryption session if no one set in this->start
    aes_decryption_stream(const std::string& key = {}, const std::string& additional_authenticated_data = {});
    ~aes_decryption_stream();

    //start decryption session. Key is required here or in constructor
    void start(const std::string& key = {}, const std::string& marker = {});
    std::string decrypt(const std::string& cipher_chunk);
    //finalize decryption session and check input tag
    void finalize(const aes_tag_type& tag);

private:
    std::unique_ptr<impl::aes_decryption_stream_impl> _impl;
};

//Improve crypto resistance by using PBKDF2
//

salted_key_type aes_create_salted_key(const std::string& user_key);
std::string aes_get_salted_key(const std::string& user_key, const std::string& salt);
std::string aes_get_salted_key(const std::string& user_key, const aes_salt_type& salt);

//Encrypt data at once. It can be provide authenticity of data with custom create_check_tag function
//

std::string aes_encrypt(const std::string& key, const std::string& plain_data);
std::string aes_encrypt(const std::string& key, const std::string& plain_data,
                        std::function<std::string(const std::string& key, const std::string& cipher_data)> create_check_tag,
                        std::string& check_tag);

std::string aes_decrypt(const std::string& key, const std::string& cipher_data);
std::string aes_decrypt(const std::string& key, const std::string& cipher_data, const std::string& check_tag,
                        std::function<std::string(const std::string& key, const std::string& cipher_data)> create_check_tag);

//Encrypt file with key and providing check tag.
//Use 'marker' like file type marker.
//Warning:
//  on POSIX systems you can happily read and write a file already opened by another process.
//  Therefore lock file writing before encrypt this to prevent corruption!
//

// return check tag
aes_tag_type aes_encrypt_file(const std::string& path, const std::string& key, const std::string& marker = {});
void aes_decrypt_file(const std::string& path, const std::string& key, const aes_tag_type& tag, const std::string& marker = {});

//Method to transfer as encrypted data and it key through unencrypted network
//To make this 'miracle' data is transferred with three chunks separated in time
//useless individually. This chunks are not classical cipher data, initialization vector
//and cipher key to prevent easy reveal

//Warning:
//  Cipher data (1.), session data (2.) and instant key (3.) should
//  be pass via different data channels.

using flip_session_type = std::pair<std::string /*cipher data*/, std::string /*session data*/>;

flip_session_type aes_ecnrypt_flip(const std::string& plain_data,
                                   const std::string& user_key,
                                   const std::string& marker = {},
                                   bool add_garbage = false);
std::string aes_decrypt_flip(const std::string& cipher_data,
                             const std::string& user_key,
                             const std::string& session_data,
                             const std::string& marker = {});

} // namespace ssl_helpers
