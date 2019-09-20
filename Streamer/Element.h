#pragma once

#include "Module.h"

#include <interfaces/IStream.h>

namespace WPEFramework {

namespace Player {

    namespace Implementation {

        struct ElementInfo {
            ElementInfo(Exchange::IStream::IElement::type type) 
                : Type(type) 
            {
            }

            Exchange::IStream::IElement::type Type;
        };

        class Element : public Exchange::IStream::IElement {
        public:
            Element() = delete;
            Element(const Element&) = delete;
            Element& operator=(const Element&) = delete;   

            Element(const ElementInfo& info)
                : _elementInfo(info)
            {                    
            }
            ~Element() override
            {
            }
            Exchange::IStream::IElement::type Type() const override
            {
                return _elementInfo.Type;
            }

            BEGIN_INTERFACE_MAP(Element)
            INTERFACE_ENTRY(Exchange::IStream::IElement)
            END_INTERFACE_MAP

        private:
            ElementInfo _elementInfo;
        };

        class ElementIterator : public Exchange::IStream::IElement::IIterator {
        public:
            ElementIterator() = delete;
            ElementIterator(const ElementIterator&) = delete;
            ElementIterator& operator=(const ElementIterator&) = delete;            

            ElementIterator(const std::list<Element*>& source)
            {
                Reset();
                std::list<Element*>::const_iterator index = source.begin();
                while (index != source.end()) {
                    Exchange::IStream::IElement* element = (*index);
                    element->AddRef();
                    _list.push_back(element);
                    index++;
                }
            }
            ~ElementIterator() override
            {
                while (_list.size() != 0) {
                    _list.front()->Release();
                    _list.pop_front();
                }                
            }
            void Reset() override
            {
                _index = 0;
            }                
            bool IsValid() const override
            {
                return ((_index != 0) && (_index <= _list.size()));
            }
            bool Next() override
            {
                if (_index == 0) {
                    _index = 1;
                    _iterator = _list.begin();
                } else if (_index <= _list.size()) {
                    _index++;
                    _iterator++;
                }
                return (IsValid());
            }
            Exchange::IStream::IElement* Current() override
            {
                ASSERT(IsValid() == true);
                Exchange::IStream::IElement* result = nullptr;
                result = (*_iterator);
                ASSERT(result != nullptr);
                result->AddRef();
                return (result);
            }

            BEGIN_INTERFACE_MAP(ElementIterator)
            INTERFACE_ENTRY(Exchange::IStream::IElement::IIterator)
            END_INTERFACE_MAP

        private:
            uint32_t _index;
            std::list<Exchange::IStream::IElement*> _list;
            std::list<Exchange::IStream::IElement*>::iterator _iterator;
        };
    }
}

}
