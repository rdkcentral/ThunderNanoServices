/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2020 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#pragma once

#include "Module.h"

#include <interfaces/IStream.h>

namespace WPEFramework {

namespace Player {

    namespace Implementation {

        struct ElementaryStream {
            ElementaryStream(Exchange::IStream::IElement::type type)
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

            Element(const ElementaryStream& es)
                : _es(es)
            {
            }
            ~Element() override
            {
            }
            Exchange::IStream::IElement::type Type() const override
            {
                return _es.Type;
            }

            BEGIN_INTERFACE_MAP(Element)
            INTERFACE_ENTRY(Exchange::IStream::IElement)
            END_INTERFACE_MAP

        private:
            ElementaryStream _es;
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
