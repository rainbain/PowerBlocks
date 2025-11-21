/**
 * @file cpu.h
 * @brief Macro definitions for the Broadway CPU.
 *
 * Macro definitions for the Broadway CPU.
 *
 * @author Michael Garofalo (Techflash)
 * @date 2025
 * @license MIT (see LICENSE file)
 */

#pragma once

/* SPR Numbers */
#define IBAT0U 528
#define IBAT0L 529
#define IBAT1U 530
#define IBAT1L 531
#define IBAT2U 532
#define IBAT2L 533
#define IBAT3U 534
#define IBAT3L 535

#define DBAT0U 536
#define DBAT0L 537
#define DBAT1U 538
#define DBAT1L 539
#define DBAT2U 540
#define DBAT2L 541
#define DBAT3U 542
#define DBAT3L 543

#define IBAT4U 560
#define IBAT4L 561
#define IBAT5U 562
#define IBAT5L 563
#define IBAT6U 564
#define IBAT6L 565
#define IBAT7U 566
#define IBAT7L 567

#define DBAT4U 568
#define DBAT4L 569
#define DBAT5U 570
#define DBAT5L 571
#define DBAT6U 572
#define DBAT6L 573
#define DBAT7U 574
#define DBAT7L 575

#define HID0   1008
#define HID4   1011


/* MSR values */
#define MSR_EE (1 << (31 - 16))
#define MSR_PR (1 << (31 - 17))
#define MSR_FP (1 << (31 - 18))
#define MSR_IR (1 << (31 - 26))
#define MSR_DR (1 << (31 - 27))

/* HID0 values */
#define HID0_ICE  (1 << (31 - 16))
#define HID0_DCE  (1 << (31 - 17))
#define HID0_ICFI (1 << (31 - 20))
#define HID0_DCFI (1 << (31 - 21))

/* HID4 values */
#define HID4_SBE (1 << (31 - 6))
