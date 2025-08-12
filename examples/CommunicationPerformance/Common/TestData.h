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

#include <array>

// TODO: namespace

template<size_t N>
class TestData {
public :
    TestData(const TestData&) = delete;
    TestData(TestData&&) = delete;

    TestData& operator=(const TestData&) = delete;
    TestData& operator=(TestData&&) = delete;

    TestData()
        : _index{ 0 }
    {}

    ~TestData()
    {}

    bool Next(std::pair<uint16_t, uint64_t>& data)
    {
       constexpr size_t MAX_INDEX{ sizeof(series[0]) / sizeof(series[0][0]) };
       static_assert((N > 0) && (N < MAX_INDEX), "Template argument is out-of-range" );

        bool result = _index < MAX_INDEX; 

        if (result != false) {
            data = series[N-1][_index];
            ++_index;
        }

        return result;
    }

    void Reset()
    {
        _index = 0;
    }

private :

    size_t _index;

    // Avoid stack allocation, use code segment (constant) or data segment instead
    constexpr static std::array<std::array<std::pair<uint16_t, uint64_t>, 50>, 5> series
    {{ // Outer std:: array initializeri, eg aggregate initializer
          {{ // Outer std::array element initializer == inner std::array initializer, eg aggregate initializer
            // Comma separated element initializer(s)
              { 7  , 711 }
            , { 50 , 421 }
            , { 17 , 482 }
            , { 18 , 616 }
            , { 3  , 602 }
            , { 44 , 461 }
            , { 14 , 502 }
            , { 13 , 787 }
            , { 54 , 581 }
            , { 24 , 538 }
            , { 88 , 684 }
            , { 13 , 590 }
            , { 13 , 814 }
            , { 78 , 508 }
            , { 92 , 523 }
            , { 35 , 535 }
            , { 87 , 472 }
            , { 93 , 583 }
            , { 71 , 757 }
            , { 66 , 619 }
            , { 96 , 11261 }
            , { 77 , 405 }
            , { 7  , 595 }
            , { 60 , 532 }
            , { 36 , 598 }
            , { 12 , 611 }
            , { 55 , 629 }
            , { 9  , 578 }
            , { 27 , 599 }
            , { 8  , 514 }
            , { 22 , 511 }
            , { 99 , 531 }
            , { 23 , 459 }
            , { 71 , 563 }
            , { 50 , 627 }
            , { 91 , 451 }
            , { 80 , 497 }
            , { 61 , 742 }
            , { 71 , 384 }
            , { 99 , 452 }
            , { 53 , 593 }
            , { 56 , 607 }
            , { 45 , 638 }
            , { 98 , 490 }
            , { 35 , 564 }
            , { 69 , 459 }
            , { 99 , 434 }
            , { 22 , 533 }
            , { 94 , 650 }
            , { 70 , 458 }
          }}
        , {{ // Outer std::array element initializer == inner std::array initializer, eg aggregate initializer
            // Comma separated element initializer(s)
              { 60 , 474 }
            , { 66 , 537 }
            , { 82 , 476 }
            , { 80 , 765 }
            , { 8  , 447 }
            , { 26 , 468 }
            , { 77 , 424 }
            , { 46 , 641 }
            , { 52 , 587 }
            , { 96 , 583 }
            , { 8  , 599 }
            , { 17 , 460 }
            , { 97 , 500 }
            , { 17 , 469 }
            , { 65 , 565 }
            , { 74 , 511 }
            , { 77 , 541 }
            , { 12 , 450 }
            , { 45 , 633 }
            , { 0  , 593 }
            , { 71 , 631 }
            , { 98 , 418 }
            , { 5  , 563 }
            , { 63 , 565 }
            , { 21 , 521 }
            , { 29 , 473 }
            , { 13 , 558 }
            , { 31 , 650 }
            , { 77 , 402 }
            , { 1  , 475 }
            , { 46 , 504 }
            , { 5  , 444 }
            , { 99 , 580 }
            , { 61 , 612 }
            , { 17 , 489 }
            , { 40 , 500 }
            , { 52 , 581 }
            , { 59 , 498 }
            , { 51 , 540 }
            , { 36 , 411 }
            , { 88 , 805 }
            , { 59 , 465 }
            , { 18 , 475 }
            , { 17 , 567 }
            , { 76 , 524 }
            , { 49 , 499 }
            , { 91 , 488 }
            , { 50 , 529 }
            , { 93 , 566 }
            , { 1  , 408 }
          }}
        , {{ // Outer std::array element initializer == inner std::array initializer, eg aggregate initializer
            // Comma separated element initializer(s)
              { 42 , 653 }
            , { 77 , 468 }
            , { 14 , 476 }
            , { 71 , 392 }
            , { 91 , 471 }
            , { 22 , 532 }
            , { 27 , 763 }
            , { 60 , 557 }
            , { 7  , 474 }
            , { 7  , 569 }
            , { 65 , 878 }
            , { 81 , 582 }
            , { 32 , 408 }
            , { 87 , 466 }
            , { 20 , 551 }
            , { 75 , 382 }
            , { 66 , 473 }
            , { 82 , 642 }
            , { 72 , 536 }
            , { 76 , 596 }
            , { 90 , 562 }
            , { 27 , 535 }
            , { 11 , 705 }
            , { 83 , 622 }
            , { 58 , 480 }
            , { 30 , 554 }
            , { 45 , 506 }
            , { 41 , 518 }
            , { 4  , 485 }
            , { 92 , 464 }
            , { 93 , 546 }
            , { 78 , 582 }
            , { 67 , 509 }
            , { 4  , 410 }
            , { 81 , 543 }
            , { 58 , 570 }
            , { 91 , 494 }
            , { 75 , 614 }
            , { 86 , 656 }
            , { 30 , 479 }
            , { 48 , 456 }
            , { 16 , 521 }
            , { 76 , 634 }
            , { 77 , 451 }
            , { 36 , 416 }
            , { 28 , 426 }
            , { 17 , 713 }
            , { 2  , 538 }
            , { 11 , 595 }
            , { 90  ,535 }
          }}
        , {{ // Outer std::array element initializer == inner std::array initializer, eg aggregate initializer
            // Comma separated element initializer(s)
              { 49 , 44117 }
            , { 68 , 766 }
            , { 31 , 703 }
            , { 11 , 691 }
            , { 43 , 706 }
            , { 12 , 796 }
            , { 58 , 707 }
            , { 24 , 766 }
            , { 84 , 806 }
            , { 62 , 751 }
            , { 94 , 614 }
            , { 26 , 787 }
            , { 13 , 798 }
            , { 25 , 720 }
            , { 44 , 771 }
            , { 14 , 632 }
            , { 54 , 705 }
            , { 51 , 793 }
            , { 71 , 648 }
            , { 12 , 684 }
            , { 80 , 743 }
            , { 81 , 643 }
            , { 72 , 655 }
            , { 86 , 607 }
            , { 21 , 693 }
            , { 92 , 642 }
            , { 19 , 602 }
            , { 57 , 651 }
            , { 52 , 684 }
            , { 33 , 684 }
            , { 14 , 761 }
            , { 33 , 662 }
            , { 67 , 738 }
            , { 77 , 609 }
            , { 76 , 747 }
            , { 42 , 794 }
            , { 54 , 773 }
            , { 34 , 747 }
            , { 99 , 6123 }
            , { 36 , 821 }
            , { 29 , 838 }
            , { 93 , 694 }
            , { 62 , 750 }
            , { 42 , 910 }
            , { 50 , 795 }
            , { 38 , 731 }
            , { 88 , 623 }
            , { 69 , 783 }
            , { 54 , 650 }
            , { 56 , 609 }
          }}
        , {{ // Outer std::array element initializer == inner std::array initializer, eg aggregate initializer
            // Comma separated element initializer(s)
              { 99 , 896 }
            , { 80 , 782 }
            , { 69 , 640 }
            , { 8  , 807 }
            , { 45 , 755 }
            , { 57 , 588 }
            , { 70 , 710 }
            , { 19 , 815 }
            , { 13 , 655 }
            , { 79 , 604 }
            , { 64 , 615 }
            , { 89 , 580 }
            , { 14 , 807 }
            , { 29 , 617 }
            , { 7  , 705 }
            , { 24 , 619 }
            , { 32 , 640 }
            , { 10 , 603 }
            , { 12 , 632 }
            , { 35 , 771 }
            , { 14 , 706 }
            , { 15 , 788 }
            , { 62 , 683 }
            , { 82 , 650 }
            , { 90 , 672 }
            , { 37 , 670 }
            , { 93 , 951 }
            , { 16 , 730 }
            , { 26 , 720 }
            , { 96 , 835 }
            , { 10 , 687 }
            , { 92 , 748 }
            , { 8  , 726 }
            , { 79 , 643 }
            , { 65 , 750 }
            , { 53 , 577 }
            , { 2  , 605 }
            , { 35 , 772 }
            , { 72 , 29689 }
            , { 47 , 1485 }
            , { 47 , 498 }
            , { 2  , 481 }
            , { 1  , 432 }
            , { 61 , 531 }
            , { 63 , 588 }
            , { 41 , 551 }
            , { 50 , 476 }
            , { 27 , 403 }
            , { 51 , 564 }
            , { 94 , 483 }
          }}
    }};
};

template <size_t N>
constexpr std::array<std::array<std::pair<uint16_t, uint64_t>, 50>, 5> TestData<N>::series;
