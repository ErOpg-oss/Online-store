CREATE TRIGGER recalc_order_price_trigger
AFTER UPDATE OF price ON products
FOR EACH ROW
EXECUTE FUNCTION trg_recalculate_order_price();

CREATE TRIGGER audit_orders_trigger
AFTER INSERT OR UPDATE OR DELETE ON orders
FOR EACH ROW
EXECUTE FUNCTION trg_audit_orders();

CREATE TRIGGER order_status_history_trigger
AFTER UPDATE OF status ON orders
FOR EACH ROW
EXECUTE FUNCTION trg_order_status_history();

CREATE TRIGGER audit_products_trigger
AFTER INSERT OR UPDATE OR DELETE ON products
FOR EACH ROW
EXECUTE FUNCTION trg_audit_products();

CREATE TRIGGER audit_users_trigger
AFTER INSERT OR UPDATE OR DELETE ON users
FOR EACH ROW
EXECUTE FUNCTION trg_audit_users();
