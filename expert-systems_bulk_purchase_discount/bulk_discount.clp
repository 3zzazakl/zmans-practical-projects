(deftemplate purchase (slot item) (slot price) (slot quantity))
(deftemplate discount-threshold (slot min-quantity))
(assert (discount-threshold (min-quantity 3)))
(assert (purchase (item Laptop) (price 15000) (quantity 1)))
(assert (purchase (item Notebook) (price 50) (quantity 5)))
(defrule bulk-purchase-discount
  (purchase (item ?item) (price ?price) (quantity ?qty))
  (discount-threshold (min-quantity ?threshold))
  (test (>= ?qty ?threshold))
  =>
  (bind ?total (* ?price ?qty))
  (bind ?discounted-total (* ?total 0.9))
  (printout t ?item " discounted total: " ?discounted-total " EGP" crlf))
