/*
 * Copyright (C) 2010-2012  The Async HBase Authors.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *   - Neither the name of the StumbleUpon nor the names of its contributors
 *     may be used to endorse or promote products derived from this software
 *     without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
package org.apache.kudu.client;

import com.stumbleupon.async.DeferredGroupException;
import com.stumbleupon.async.TimeoutException;
import org.apache.kudu.annotations.InterfaceAudience;
import org.apache.kudu.annotations.InterfaceStability;

import java.io.IOException;

/**
 * The parent class of all exceptions sent by the Kudu client. This is the only exception you will
 * see if you're using the non-async API, such as {@link KuduSession} instead of
 * {@link AsyncKuduSession}.
 *
 * Each instance of this class has a {@link Status} which gives more information about the error.
 */
@InterfaceAudience.Public
@InterfaceStability.Evolving
@SuppressWarnings("serial")
public abstract class KuduException extends IOException {

  private final Status status;

  /**
   * Constructor.
   * @param status object containing the reason for the exception
   * trace.
   */
  KuduException(Status status) {
    super(status.getMessage());
    this.status = status;
  }

  /**
   * Constructor.
   * @param status object containing the reason for the exception
   * @param cause The exception that caused this one to be thrown.
   */
  KuduException(Status status, Throwable cause) {
    super(status.getMessage(), cause);
    this.status = status;
  }

  /**
   * Get the Status object for this exception.
   * @return a status object indicating the reason for the exception
   */
  public Status getStatus() {
    return status;
  }

  /**
   * Inspects the given exception and transforms it into a KuduException.
   * @param e generic exception we want to transform
   * @return a KuduException that's easier to handle
   */
  static KuduException transformException(Exception e) {
    if (e instanceof KuduException) {
      return (KuduException) e;
    } else if (e instanceof DeferredGroupException) {
      // TODO anything we can do to improve on that kind of exception?
    } else if (e instanceof TimeoutException) {
      Status statusTimeout = Status.TimedOut(e.getMessage());
      return new NonRecoverableException(statusTimeout, e);
    } else if (e instanceof InterruptedException) {
      // Need to reset the interrupt flag since we caught it but aren't handling it.
      Thread.currentThread().interrupt();
      Status statusAborted = Status.Aborted(e.getMessage());
      return new NonRecoverableException(statusAborted, e);
    }
    Status status = Status.IOError(e.getMessage());
    return new NonRecoverableException(status, e);
  }
}
