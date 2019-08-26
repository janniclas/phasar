/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Printer.h
 *
 *  Created on: 07.09.2018
 *      Author: rleer
 */

#ifndef PHASAR_PHASARLLVM_UTILS_PRINTER_H_
#define PHASAR_PHASARLLVM_UTILS_PRINTER_H_

#include <ostream>
#include <sstream>
#include <string>

namespace psr {

template <typename N> struct NodePrinter {
  NodePrinter() = default;
  NodePrinter(const NodePrinter &) = delete;
  NodePrinter &operator=(const NodePrinter &) = delete;
  NodePrinter(NodePrinter &&) = delete;
  NodePrinter &operator=(NodePrinter &&) = delete;
  virtual ~NodePrinter() = default;

  virtual void printNode(std::ostream &os, N n) const = 0;

  virtual std::string NtoString(N n) const {
    std::stringstream ss;
    printNode(ss, n);
    return ss.str();
  }
};

template <typename D> struct DataFlowFactPrinter {
  DataFlowFactPrinter() = default;
  DataFlowFactPrinter(const DataFlowFactPrinter &) = delete;
  DataFlowFactPrinter &operator=(const DataFlowFactPrinter &) = delete;
  DataFlowFactPrinter(DataFlowFactPrinter &&) = delete;
  DataFlowFactPrinter &operator=(DataFlowFactPrinter &&) = delete;
  virtual ~DataFlowFactPrinter() = default;

  virtual void printDataFlowFact(std::ostream &os, D d) const = 0;

  virtual std::string DtoString(D d) const {
    std::stringstream ss;
    printDataFlowFact(ss, d);
    return ss.str();
  }
};

template <typename V> struct ValuePrinter {
  ValuePrinter() = default;
  ValuePrinter(const ValuePrinter &) = delete;
  ValuePrinter &operator=(const ValuePrinter &) = delete;
  ValuePrinter(ValuePrinter &&) = delete;
  ValuePrinter &operator=(ValuePrinter &&) = delete;
  virtual ~ValuePrinter() = default;

  virtual void printValue(std::ostream &os, V v) const = 0;

  virtual std::string VtoString(V v) const {
    std::stringstream ss;
    printValue(ss, v);
    return ss.str();
  }
};

template <typename M> struct MethodPrinter {
  MethodPrinter() = default;
  MethodPrinter(const MethodPrinter &) = delete;
  MethodPrinter &operator=(const MethodPrinter &) = delete;
  MethodPrinter(MethodPrinter &&) = delete;
  MethodPrinter &operator=(MethodPrinter &&) = delete;
  virtual ~MethodPrinter() = default;

  virtual void printMethod(std::ostream &os, M m) const = 0;

  virtual std::string MtoString(M m) const {
    std::stringstream ss;
    printMethod(ss, m);
    return ss.str();
  }
};

} // namespace psr

#endif
