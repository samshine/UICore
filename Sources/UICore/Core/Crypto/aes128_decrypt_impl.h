/*
**  UICore
**  Copyright (c) 1997-2015 The UICore Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries UICore may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Mark Page
*/

#pragma once

#include <cstdint>
#include "UICore/Core/System/databuffer.h"
#include "UICore/Core/Crypto/aes128_decrypt.h"
#include "aes_impl.h"

namespace uicore
{
	class AES128_Decrypt_Impl : public AES128_Decrypt, public AES_Impl
	{
	public:
		AES128_Decrypt_Impl();

		/// \brief Get decrypted data
		///
		/// This is the databuffer used internally to store the decrypted data.
		/// You may call "set_size()" to clear the buffer, inbetween calls to "add()"
		/// You may call "set_capacity()" to optimise storage requirements before the add() call
		DataBufferPtr data() const override;

		/// \brief Resets the decryption
		void reset() override;

		/// \brief Purge the databuffer
		///
		/// See get_data()\n
		/// reset() calls this function
		void purge_data();

		/// \brief Sets the initialisation vector
		///
		/// This must be called before the initial add()
		void set_iv(const unsigned char iv[16]) override;

		/// \brief Sets the cipher key
		///
		/// This must be called before the initial add()
		void set_key(const unsigned char key[16]) override;

		void set_padding(bool value, bool use_pkcs7) override;

		/// \brief Adds data to be decrypted
		void add(const void *data, int size) override;

		/// \brief Add data to be decrypted
		///
		/// \param data = Data Buffer
		void add(const DataBufferPtr &data) override { add(data->data(), data->size()); }

		/// \brief Finalize decryption
		///
		/// IMPORTANT, to avoid timing attacks, if this function fails, you should still validate the data (via a hash or otherwise), then throw an error
		///
		/// \return false = AES Padding value is invalid.
		bool calculate() override;

	private:
		void process_chunk();

		uint32_t key_expanded[aes128_nb_mult_nr_plus1];

		unsigned char chunk[aes128_block_size_bytes];
		uint32_t initialisation_vector_1;
		uint32_t initialisation_vector_2;
		uint32_t initialisation_vector_3;
		uint32_t initialisation_vector_4;

		int chunk_filled;

		bool initialisation_vector_set;
		bool cipher_key_set;
		bool calculated;
		bool padding_enabled;
		bool padding_pkcs7;

		DataBufferPtr databuffer = DataBuffer::create(0);
	};
}
