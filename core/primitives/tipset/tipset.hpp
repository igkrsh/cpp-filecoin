/**
 * Copyright Soramitsu Co., Ltd. All Rights Reserved.
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
#define CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP

#include "primitives/block/block.hpp"
#include "primitives/tipset/tipset_key.hpp"

namespace fc::primitives::tipset {

  enum class TipsetError : int {
    kNoBlocks = 1,        // need to have at least one block to create tipset
    kMismatchingHeights,  // cannot create tipset, mismatching blocks heights
    kMismatchingParents,  // cannot create tipset, mismatching block parents
    kTicketHasNoValue,    // optional ticket is not initialized
    kNoBeacons,
  };
}

/**
 * @brief Outcome errors declaration
 */
OUTCOME_HPP_DECLARE_ERROR(fc::primitives::tipset, TipsetError);

namespace fc::primitives::tipset {
  using block::BeaconEntry;
  using block::BlockHeader;
  using crypto::randomness::DomainSeparationTag;
  using crypto::randomness::Randomness;

  struct MessageVisitor {
    using Visitor =
        std::function<outcome::result<void>(size_t, bool bls, const CID &)>;
    outcome::result<void> visit(const BlockHeader &block,
                                const Visitor &visitor);
    IpldPtr ipld;
    std::set<CID> visited{};
  };

  /**
   * @struct Tipset implemented according to
   * https://github.com/filecoin-project/lotus/blob/6e94377469e49fa4e643f9204b6f46ef3cb3bf04/chain/types/tipset.go#L18
   */
  struct Tipset {
    static outcome::result<Tipset> create(std::vector<BlockHeader> blocks);

    static outcome::result<Tipset> load(Ipld &ipld,
                                        const std::vector<CID> &cids);

    outcome::result<Tipset> loadParent(Ipld &ipld) const;

    outcome::result<BeaconEntry> latestBeacon(Ipld &ipld) const;

    outcome::result<void> visitMessages(
        IpldPtr ipld, const MessageVisitor::Visitor &visitor) const;

    outcome::result<BigInt> nextBaseFee(IpldPtr ipld) const;

    outcome::result<Randomness> beaconRandomness(
        Ipld &ipld,
        DomainSeparationTag tag,
        ChainEpoch round,
        gsl::span<const uint8_t> entropy) const;

    outcome::result<Randomness> ticketRandomness(
        Ipld &ipld,
        DomainSeparationTag tag,
        ChainEpoch round,
        gsl::span<const uint8_t> entropy) const;

    /**
     * @brief makes key of cids
     */
    outcome::result<TipsetKey> makeKey() const;

    /**
     * @return key made of parents
     */
    TipsetKey getParents() const;

    /**
     * @return min timestamp
     */
    uint64_t getMinTimestamp() const;

    /**
     * @return min ticket block
     */
    const block::BlockHeader &getMinTicketBlock() const;

    /**
     * @return parent state root
     */
    const CID &getParentStateRoot() const;

    const CID &getParentMessageReceipts() const;

    /**
     * @return parent weight
     */
    const BigInt &getParentWeight() const;

    const BigInt &getParentBaseFee() const;

    /**
     * @brief checks whether tipset contains block by cid
     * @param cid content identifier to look for
     * @return true if contains, false otherwise
     */
    bool contains(const CID &cid) const;

    std::vector<CID> cids;                 ///< block cids
    std::vector<block::BlockHeader> blks;  ///< block headers
    uint64_t height{};                     ///< height
  };

  /**
   * @brief compares two Tipset instances
   * @param lhs first tipset
   * @param rhs second tipset
   * @return true if equal, false otherwise
   */
  bool operator==(const Tipset &l, const Tipset &r);

  /**
   * @brief compares two Tipset instances
   * @param lhs first tipset
   * @param rhs second tipset
   * @return false if equal, true otherwise
   */
  bool operator!=(const Tipset &l, const Tipset &r);

  CBOR_TUPLE(Tipset, cids, blks, height)

  /**
   * @brief change type
   */
  enum class HeadChangeType : int { REVERT, APPLY, CURRENT };

  /**
   * @struct HeadChange represents atomic chain change
   */
  struct HeadChange {
    HeadChangeType type;
    Tipset value;
  };
}  // namespace fc::primitives::tipset

#endif  // CPP_FILECOIN_CORE_PRIMITIVES_TIPSET_TIPSET_HPP
