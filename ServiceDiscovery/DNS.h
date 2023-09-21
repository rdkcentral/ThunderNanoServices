#include "Module.h"
#include <core/core.h>

namespace WPEFramework {
namespace Plugin {
    namespace DNS {
        /***
         * RFC1035
         * A four bit field that specifies kind of query in this message.
         * This value is set by the originator of a query and copied
         * into the response.
         */
        enum class OperationCode : uint8_t {
            Query = 0, // a standard query (QUERY)
            IQuery = 1, // an inverse query (IQUERY)
            Status = 2 // a server status request (STATUS)
        };

        /**
         * Response code - this 4 bit field is set as part of responses.
         */
        enum class ResponseCode : uint8_t {
            NoError = 0,
            FormatError = 1,
            ServerFailure = 2,
            NameError = 3,
            NotImplemented = 4,
            Refused = 5
        };

        // RFC 1035: 3.2.2-3 TYPE and QTYPE values
        enum class type : uint8_t {
            A = 1, // a host address
            NS = 2, // an authoritative name server
            MD = 3, // a mail destination (Obsolete - use MX)
            MF = 4, // a mail forwarder (Obsolete - use MX)
            CNAME = 5, // the canonical name for an alias
            SOA = 6, // marks the start of a zone of authority
            MB = 7, // a mailbox domain name (EXPERIMENTAL)
            MG = 8, // a mail group member (EXPERIMENTAL)
            MR = 9, // a mail rename domain name (EXPERIMENTAL)
            Null = 10, // a null RR (EXPERIMENTAL)
            WKS = 11, // a well known service description
            PTR = 12, // a domain name pointer
            HINFO = 13, // host information
            MINFO = 14, // mailbox or mail list information
            MX = 15, // mail exchange
            TXT = 16, // text strings
            // QTypes
            AXFR = 252, // A request for a transfer of an entire zone
            MAILB = 253, // A request for mailbox-related records (MB, MG or MR)
            MAILA = 254, // A request for mail agent RRs (Obsolete - see MX)
            ALL = 255 // A request for all records
        };

        enum class klass : uint8_t {
            IN = 1, // the Internet
            CS = 2, // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
            CH = 3, // the CHAOS class
            HS = 4, // Hesiod [Dyer 87]
            // QClass
            ALL = 255 // any class
        };

        class BaseFrame {
        private:
            static constexpr uint8_t TransactionIdOffset = 0;
            static constexpr uint8_t FlagsOffset = 2;
            static constexpr uint8_t QueryCountOffset = 4;
            static constexpr uint8_t AnswerCountOffset = 6;
            static constexpr uint8_t AuthorityCountOffset = 8;
            static constexpr uint8_t AdditionalCountOffset = 10;

            static constexpr uint16_t OperationCodeMask = 0x001E; // RFC 1035
            static constexpr uint16_t ResponseCodeMask = 0xF000; // RFC 1035

            static constexpr uint16_t FlagsBitMask = 0x0DE1; // RFC 1035 & RFC 2535
            static constexpr uint16_t ResponseBitMask = 0x0001; // RFC 1035
            static constexpr uint16_t AuthoritativeAnswerBitMask = 0x0010; // RFC 1035
            static constexpr uint16_t TruncationBitMask = 0x0020; // RFC 1035
            static constexpr uint16_t RecursionDesiredBitMask = 0x0040; // RFC 1035
            static constexpr uint16_t RecursionAvailableBitMask = 0x0080; // RFC 1035
            static constexpr uint16_t AuthenticDataBitMask = 0x0200; // RFC 2535
            static constexpr uint16_t CheckingDisabledBitMask = 0x0400; // RFC 2535

            static constexpr uint8_t OperationCodeOffset = 1; // bits
            static constexpr uint8_t ResponseCodeOffset = 12; // bits

            static constexpr uint8_t HeaderLength = 12; // bytes
            using Buffer = Core::FrameType<HeaderLength, true, uint16_t>;

        public:
            template <typename TYPE>
            struct IIterator {
                virtual ~IIterator() = default;

                virtual bool IsValid() const = 0;
                virtual void Reset() = 0;
                virtual bool Next() = 0;

                virtual TYPE* Current() = 0;
            };

            class Question {
            public:
                class Iterator : public IIterator<Question> {
                public:
                    ~Iterator() = default;

                    Iterator(const BaseFrame& parent)
                        : _parent(parent)
                    {
                    }

                    bool IsValid() const override
                    {
                        return false;
                    }

                    void Reset() override
                    {
                    }
                    bool Next() override
                    {
                        return false;
                    }

                    Question* Current() override
                    {
                        return nullptr;
                    }

                private:
                    const BaseFrame& _parent;
                };

                const std::string& Name()
                {
                    return _name;
                }

                type Type() const
                {
                    return _qtype;
                }

                klass Class() const
                {
                    return _qclass;
                }

                ~Question() = default;

            private:
                std::string _name;
                type _qtype;
                klass _qclass;
            };

            class ResourceRecord {
            public:
                class Iterator : public IIterator<ResourceRecord> {
                    ~Iterator() = default;

                    bool IsValid() const override
                    {
                        return false;
                    }

                    void Reset() override
                    {
                    }
                    bool Next() override
                    {
                        return false;
                    }

                    ResourceRecord* Current() override
                    {
                        return nullptr;
                    }
                };

                ~ResourceRecord() = default;

                type Type() const
                {
                    return _qtype;
                }

                klass Class() const
                {
                    return _qclass;
                }

            private:
                type _qtype;
                klass _qclass;
            };

            BaseFrame()
            {
            }

            BaseFrame(const uint8_t buffer[], const uint16_t length)
            {
                Deserialize(0, buffer, length);
            }

            ~BaseFrame() = default;

            inline uint16_t TransactionId() const
            {
                return GetNumber<uint16_t>(TransactionIdOffset);
            }

            inline bool IsAuthoritativeAnswer() const
            {
                return (Flags() & AuthoritativeAnswerBitMask) == AuthoritativeAnswerBitMask;
            }

            inline bool IsTruncated() const
            {
                return (Flags() & TruncationBitMask) == TruncationBitMask;
            }

            inline bool IsRecursionDesired() const
            {
                return (Flags() & RecursionDesiredBitMask) == RecursionDesiredBitMask;
            }

            inline bool IsRecursionAvailable() const
            {
                return (Flags() & RecursionAvailableBitMask) == RecursionAvailableBitMask;
            }

            inline bool IsAuthenticData() const
            {
                return (Flags() & AuthenticDataBitMask) == AuthenticDataBitMask;
            }

            inline bool IsCheckingDisabled() const
            {
                return (Flags() & CheckingDisabledBitMask) == CheckingDisabledBitMask;
            }

            inline bool IsQuery() const
            {
                return (Flags() & ResponseBitMask) == 0;
            }

            inline OperationCode Operation() const
            {
                return static_cast<OperationCode>((GetNumber<uint16_t>(FlagsOffset) & OperationCodeMask) >> OperationCodeOffset);
            }

            inline ResponseCode Response() const
            {
                return static_cast<ResponseCode>((GetNumber<uint16_t>(FlagsOffset) & ResponseCodeMask) >> ResponseCodeOffset);
            }

            inline uint16_t Size() const
            {
                return _buffer.Size();
            }
            inline const uint8_t* Data() const
            {
                return _buffer.Data();
            }

            IIterator<Question>* Questions()
            {
                IIterator<Question>* questions(nullptr);

                uint16_t questionCount = GetNumber<uint16_t>(QueryCountOffset);

                if (questionCount > 0) {
                    questions = new Question::Iterator(*this); //(Core::FrameType<HeaderLength, true, uint16_t> _buffer[12], questionCount);
                }

                return questions;
            }

        private:
            inline uint16_t Serialize(const uint32_t offset, uint8_t buffer[], const uint16_t length) const
            {
                uint16_t size = ((_buffer.Size() - offset) > length ? length : (_buffer.Size() - offset));
                ::memcpy(buffer, &(_buffer[offset]), size);

                return (size);
            }
            inline uint16_t Deserialize(const uint32_t offset, const uint8_t buffer[], const uint16_t length)
            {
                ASSERT(offset == _buffer.Size());

                _buffer.Size(length + _buffer.Size());
                ::memcpy(&(_buffer[offset]), buffer, length);

                return (length);
            }

            inline uint16_t Flags() const
            {
                return GetNumber<uint16_t>(FlagsOffset) & FlagsBitMask;
            }

            template <typename TYPE>
            inline TYPE GetNumber(const uint16_t offset) const
            {
                ASSERT(_buffer.Size() >= (offset + sizeof(TYPE)));

                TYPE result;

                _buffer.template GetNumber<TYPE>(offset, result);

                return result;
            }

        private:
            Buffer _buffer;
        };
    } // namespace DNS
} // namespace Plugin
} // namespace WPEFramework
