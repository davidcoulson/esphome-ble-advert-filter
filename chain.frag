  // This is the only point where a packet can be suppressed without it crossing
  // the network, so every filter lives here rather than downstream.

  // Explicitly protected addresses bypass every filter, including the RSSI
  // threshold: these are the tracked tags, and a tag being far from *this*
  // proxy is exactly the reading the tracker needs to place it near another.
  // Cheap enough to run first - the list is a handful of entries.
  bool protected_addr = false;
  for (const uint64_t allowed : this->mac_allowlist_) {
    if (allowed == adv.address) {
      protected_addr = true;
      break;
    }
  }

  // A device advertising an allowlisted service UUID is protected exactly like
  // an allowlisted MAC. This has to run here, ahead of the address-type tests,
  // because the case it exists for is a device in pairing mode advertising from
  // a rotating private address: by the time those tests run the advertisement is
  // already gone, and its address could not have been allowlisted in advance.
  //
  // It is deliberately placed before the RSSI test as well. A device being
  // paired is normally close by, but a pairing window is short and
  // user-initiated, so a missed advertisement costs a retry while the extra
  // traffic lasts only as long as the pairing does.
  //
  // Cost: this walks the payload, which the filters below otherwise defer to
  // last. Guarded on a non-empty allowlist so a build that does not use the
  // option keeps the original ordering and pays nothing.
  if (!protected_addr && (!this->service_uuid_allowlist_.empty() || !this->service_uuid128_.empty()) &&
      this->payload_has_allowed_service_uuid_(adv.data, adv.data_len)) {
    protected_addr = true;
    this->adv_allowed_service_uuid_++;
    ESP_LOGVV(TAG, "Allowing packet from %012" PRIX64 ": allowlisted service UUID", adv.address);
  }

  // Distance first, and it applies to everything else: a far-away device is not
  // worth forwarding even when we would otherwise allow it through below.
  // Cheapest test too, so nothing distant ever reaches the AES.
  if (!protected_addr && adv.rssi < this->rssi_threshold_) {
    this->adv_dropped_++;
    ESP_LOGVV(TAG, "Dropping packet from %012" PRIX64 ": RSSI %d dB below threshold %d dB", adv.address, adv.rssi,
              this->rssi_threshold_);
    return false;
  }

  // A non-resolvable private address rotates and carries no identity, so it can
  // never be matched to a device - not even with an IRK. Nothing can be done
  // with these, and each rotation looks like a brand new device downstream.
  if (!protected_addr && this->drop_non_resolvable_ &&
      this->address_is_non_resolvable_(adv.address, adv.addr_type)) {
    this->adv_dropped_++;
    ESP_LOGVV(TAG, "Dropping packet from %012" PRIX64 ": non-resolvable private address", adv.address);
    return false;
  }

  // A Resolvable Private Address we cannot resolve belongs to somebody else's
  // phone or watch: it rotates, so it can never be tracked here and is pure
  // noise. Devices with fixed addresses are left alone - they have no IRK.
  //
  // allow_espressif exempts our own ESPs from this test. Note it is a safety
  // net, not load-bearing: ESPHome advertises on the public MAC, and a public
  // address is never an RPA, so those advertisements do not reach this test
  // anyway. It only bites if an ESP is ever configured to advertise randomly.
  if (!protected_addr && !this->irks_.empty() && this->address_is_rpa_(adv.address, adv.addr_type) &&
      !(this->allow_espressif_ && this->is_espressif_oui_(adv.address))) {
    if (this->irk_matches_(adv.address)) {
      // One of ours. Mark it protected so the payload filters below cannot
      // discard it - our phones and watches advertise Apple manufacturer data,
      // which a manufacturer_blocklist entry would otherwise match.
      protected_addr = true;
    } else {
      this->adv_dropped_++;
      this->adv_dropped_rpa_++;
      ESP_LOGVV(TAG, "Dropping packet from %012" PRIX64 ": unresolved RPA", adv.address);
      return false;
    }
  }

  // Payload-based filters last: this is the only test that has to walk the
  // advertisement, and by here most traffic has already been rejected.
  if (!protected_addr && (!this->name_blocklist_.empty() || !this->manufacturer_blocklist_.empty()) &&
      this->payload_blocked_(adv.data, adv.data_len)) {
    this->adv_dropped_++;
    ESP_LOGVV(TAG, "Dropping packet from %012" PRIX64 ": blocklisted name or manufacturer", adv.address);
    return false;
  }
  this->adv_forwarded_++;

