/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/executor/remote_command_response.h"

#include "mongo/bson/simple_bsonobj_comparator.h"
#include "mongo/rpc/reply_interface.h"
#include "mongo/util/mongoutils/str.h"

namespace mongo {
namespace executor {

RemoteCommandResponse::RemoteCommandResponse(ErrorCodes::Error code, std::string reason)
    : status(code, reason){};

RemoteCommandResponse::RemoteCommandResponse(ErrorCodes::Error code,
                                             std::string reason,
                                             Milliseconds millis)
    : elapsedMillis(millis), status(code, reason) {}

RemoteCommandResponse::RemoteCommandResponse(Status s) : status(std::move(s)) {
    invariant(!isOK());
};

RemoteCommandResponse::RemoteCommandResponse(Status s, Milliseconds millis)
    : elapsedMillis(millis), status(std::move(s)) {
    invariant(!isOK());
};

RemoteCommandResponse::RemoteCommandResponse(BSONObj dataObj, Milliseconds millis)
    : data(std::move(dataObj)), elapsedMillis(millis) {
    // The buffer backing the default empty BSONObj has static duration so it is effectively
    // owned.
    invariant(data.isOwned() || data.objdata() == BSONObj().objdata());
};

RemoteCommandResponse::RemoteCommandResponse(Message messageArg,
                                             BSONObj dataObj,
                                             Milliseconds millis)
    : message(std::make_shared<const Message>(std::move(messageArg))),
      data(std::move(dataObj)),
      elapsedMillis(millis) {
    if (!data.isOwned()) {
        data.shareOwnershipWith(message->sharedBuffer());
    }
}

// TODO(amidvidy): we currently discard output docs when we use this constructor. We should
// have RCR hold those too, but we need more machinery before that is possible.
RemoteCommandResponse::RemoteCommandResponse(const rpc::ReplyInterface& rpcReply,
                                             Milliseconds millis)
    : RemoteCommandResponse(rpcReply.getCommandReply(), std::move(millis)) {}

bool RemoteCommandResponse::isOK() const {
    return status.isOK();
}

std::string RemoteCommandResponse::toString() const {
    return str::stream() << "RemoteResponse -- "
                         << " cmd:" << data.toString();
}

bool RemoteCommandResponse::operator==(const RemoteCommandResponse& rhs) const {
    if (this == &rhs) {
        return true;
    }
    SimpleBSONObjComparator bsonComparator;
    return bsonComparator.evaluate(data == rhs.data) && elapsedMillis == rhs.elapsedMillis;
}

bool RemoteCommandResponse::operator!=(const RemoteCommandResponse& rhs) const {
    return !(*this == rhs);
}

std::ostream& operator<<(std::ostream& os, const RemoteCommandResponse& response) {
    return os << response.toString();
}

}  // namespace executor
}  // namespace mongo
