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

        enum class ResourceType {
            Question = 0x00,
            Answer,
            Authority,
            Additional
        };




        // Assigned by IANA: https://www.iana.org/assignments/dns-parameters/dns-parameters.xhtml#dns-parameters-4
        enum class Type : uint16_t {
            // RFC1035
            A = 1, // a host ipv4 address
            NS = 2, // an authoritative name server
            MD = 3, // a mail destination (Obsolete - use MX)
            MF = 4, // a mail forwarder (Obsolete - use MX)
            CNAME = 5, // the canonical name for an alias
            SOA = 6, // marks the start of a zone of authority
            MB = 7, // a mailbox domain name (EXPERIMENTAL)
            MG = 8, // a mail group member (EXPERIMENTAL)
            MR = 9, // a mail rename domain name (EXPERIMENTAL)
            NULLVALUE = 10, // a null RR (EXPERIMENTAL)
            WKS = 11, // a well known service description
            PTR = 12, // a domain name pointer
            HINFO = 13, // host information
            MINFO = 14, // mailbox or mail list information
            MX = 15, // mail exchange
            TXT = 16, // text strings
            AXFR = 252, // A request for a transfer of an entire zone
            MAILB = 253, // A request for mailbox-related records (MB, MG or MR)
            MAILA = 254, // A request for mail agent RRs (Obsolete - see MX)
            ANY = 255, // A request for all records

            // RFC3596
            AAAA = 28, // a host ipv6 address

            // RFC2782
            SRV = 33, // a server selection

            // RFC-ietf-dnsop-svcb-https-12
            HTTPS = 63, // service binding type for use with HTTP
        };

        enum class Class : uint16_t {
            IN = 1, // the Internet
            CS = 2, // the CSNET class (Obsolete - used only for examples in some obsolete RFCs)
            CH = 3, // the CHAOS class
            HS = 4, // Hesiod [Dyer 87]
            // QClass
            ANY = 255 // any class
        };

        class Frame {
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

            using BufferWrapper = Core::FrameType<0, true, uint16_t>;

        public:
            // struct IResource {
            //     virtual ~IResource() = default;

            //     virtual DNS::ResourceType ResourceType() const = 0;

            //     virtual std::string Name() const = 0;
            //     virtual DNS::Type Type() const = 0;
            //     virtual DNS::Class Class() const = 0;
            // };

            template <typename TYPE>
            class Iterator {
                ~Iterator() = default;

                Iterator() = delete;
                Iterator(Iterator&&) = delete;
                Iterator(const Iterator&) = delete;
                Iterator& operator=(const Iterator&) = delete;

                Iterator(const Frame& parent) {}

                virtual void Reset() = 0;
                virtual bool IsValid() const = 0;
                virtual bool Next() = 0;

                virtual TYPE* Current() = 0;
            };

            class Question {
            public:
                ~Question() = default;

                Question() = delete;
                Question(Question&&) = delete;
                Question(const Question&) = delete;
                Question& operator=(Question&&) = delete;
                Question& operator=(const Question&) = delete;

                Question(const string& name, const DNS::Type type, const DNS::Class klass)
                    : _name(name)
                    , _type(type)
                    , _class(klass)
                {
                }

            public:
                inline const std::string& Name() const
                {
                    return _name;
                }
                inline DNS::Type Type() const
                {
                    return _type;
                }
                inline DNS::Class Class() const
                {
                    return _class;
                }

            private:
                const std::string _name;
                const DNS::Type _type;
                const DNS::Class _class;
            };

            class ResourceRecord : public Question {
            public:
                static constexpr uint32_t DefaultTTL = 60;

                virtual ~ResourceRecord() = default;

                ResourceRecord() = delete;
                ResourceRecord(ResourceRecord&&) = delete;
                ResourceRecord(const ResourceRecord&) = delete;
                ResourceRecord& operator=(ResourceRecord&&) = delete;
                ResourceRecord& operator=(const ResourceRecord&) = delete;

                ResourceRecord(const string& name, const DNS::Type type, const DNS::Class klass = DNS::Class::IN, uint32_t ttl = DefaultTTL)
                    : Question(name, type, klass)
                    , _ttl(ttl)
                {
                }

            public:
                inline uint32_t TimeToLive() const
                {
                    return 0;
                }

                inline uint16_t Serialize(const uint32_t offset, uint8_t buffer[], const uint16_t length) const
                {
                    // uint16_t size = ((_buffer.Size() - offset) > length ? length : (_buffer.Size() - offset));
                    // ::memcpy(buffer, &(_buffer[offset]), size);

                    return (0);
                }
                inline uint16_t Deserialize(const uint32_t offset, const uint8_t buffer[], const uint16_t length)
                {
                    // ASSERT(offset == _buffer.Size());

                    // _buffer.Size(length + _buffer.Size());
                    // ::memcpy(&(_buffer[offset]), buffer, length);

                    return (0);
                }

            private:
                uint32_t _ttl;
            };

            // class TextRecord : public ResourceRecord {
            //     ~TextRecord() = default;

            //     TextRecord() = delete;
            //     TextRecord(TextRecord&&) = delete;
            //     TextRecord(const TextRecord&) = delete;
            //     TextRecord& operator=(TextRecord&&) = delete;
            //     TextRecord& operator=(const TextRecord&) = delete;

            //     TextRecord(const DNS::Class klass)
            //         : ResourceRecord(DNS::Type::TXT, klass)
            //     {
            //     }
            // };

            // class ARecord : public ResourceRecord {
            //     ~ARecord() = default;

            //     ARecord() = delete;
            //     ARecord(ARecord&&) = delete;
            //     ARecord(const ARecord&) = delete;
            //     ARecord& operator=(ARecord&&) = delete;
            //     ARecord& operator=(const ARecord&) = delete;

            //     ARecord(const DNS::Class klass)
            //         : ResourceRecord(DNS::Type::A, klass)
            //     {
            //     }

            //     std::string Address()
            //     {
            //         return "0.0.0.0";
            //     }
            // };

            // class AAAARecord : public ResourceRecord {
            //     ~AAAARecord() = default;

            //     AAAARecord() = delete;
            //     AAAARecord(AAAARecord&&) = delete;
            //     AAAARecord(const AAAARecord&) = delete;
            //     AAAARecord& operator=(AAAARecord&&) = delete;
            //     AAAARecord& operator=(const AAAARecord&) = delete;

            //     AAAARecord(const DNS::Class klass)
            //         : ResourceRecord(DNS::Type::AAAA, klass)
            //     {
            //     }

            //     std::string Address()
            //     {
            //         return "2600:1901:0:38d7::";
            //     }
            // };

            Frame()
            {
            }

            Frame(const uint8_t buffer[], const uint16_t length)
            {
                Deserialize(0, buffer, length);
            }

            ~Frame() = default;

            inline uint16_t Length() const
            {
                return _buffer.Size();
            }

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

            Iterator<Question>* Questions()
            {
                Iterator<Question>* it(nullptr);

                uint16_t count = Records(DNS::ResourceType::Question);

                if (((Length() > HeaderLength)) && (count > 0)) {
                    //     questions = new Resource::Iterator(*this); //(Core::FrameType<HeaderLength, true, uint16_t> _buffer[12], questionCount);
                }

                return it;
            }

            // IIterator<IResource*>* Answers()
            // {
            //     IIterator<IResource*>* records(nullptr);

            //     uint16_t count = GetNumber<uint16_t>(AnswerCountOffset);

            //     if (count > 0) {
            //     }

            //     return records;
            // }

            // IIterator<IResource*>* AuthoritativeAnswers()
            // {
            //     IIterator<IResource*>* records(nullptr);

            //     uint16_t count = GetNumber<uint16_t>(AuthoritativeAnswerBitMask);

            //     if (count > 0) {
            //     }

            //     return records;
            // }

            // IIterator<IResource*>* AdditionalData()
            // {
            //     IIterator<IResource*>* records(nullptr);

            //     uint16_t count = GetNumber<uint16_t>(AdditionalCountOffset);

            //     if (count > 0) {
            //     }

            //     return records;
            // }

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

            inline uint16_t Records(const DNS::ResourceType type) const
            {
                return GetNumber<uint16_t>(QueryCountOffset + static_cast<const uint8_t>(type));
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
