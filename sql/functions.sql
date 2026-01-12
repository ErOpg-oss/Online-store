CREATE OR REPLACE FUNCTION get_order_status(p_order_id INTEGER)
RETURNS VARCHAR AS $$
DECLARE
    v_status VARCHAR;
BEGIN
    SELECT status INTO v_status
    FROM orders WHERE order_id = p_order_id;
    
    RETURN COALESCE(v_status, 'not_found');
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_user_order_count(p_user_id INTEGER)
RETURNS INTEGER AS $$
DECLARE
    v_count INTEGER;
BEGIN
    SELECT COUNT(*) INTO v_count
    FROM orders WHERE user_id = p_user_id;
    
    RETURN COALESCE(v_count, 0);
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_total_spent_by_user(p_user_id INTEGER)
RETURNS NUMERIC(10,2) AS $$
DECLARE
    v_total NUMERIC(10,2);
BEGIN
    SELECT COALESCE(SUM(total_price), 0) INTO v_total
    FROM orders WHERE user_id = p_user_id;
    
    RETURN v_total;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION can_return_order(p_order_id INTEGER)
RETURNS BOOLEAN AS $$
DECLARE
    v_order_date TIMESTAMP;
    v_status VARCHAR;
BEGIN
    SELECT order_date, status INTO v_order_date, v_status
    FROM orders WHERE order_id = p_order_id;

    IF NOT FOUND THEN
        RETURN FALSE;
    END IF;

    IF v_status <> 'completed' THEN
        RETURN FALSE;
    END IF;

    RETURN v_order_date >= NOW() - INTERVAL '30 days';
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_order_status_history(p_order_id INTEGER)
RETURNS TABLE(
    old_status VARCHAR,
    new_status VARCHAR,
    changed_at TIMESTAMP,
    changed_by_name VARCHAR
) AS $$
BEGIN
    RETURN QUERY
    SELECT osh.old_status,
           osh.new_status,
           osh.changed_at,
           COALESCE(u.name, 'system') as changed_by_name
    FROM order_status_history osh
    LEFT JOIN users u ON osh.changed_by = u.user_id
    WHERE osh.order_id = p_order_id
    ORDER BY osh.changed_at DESC;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION get_audit_log_by_user(p_user_id INTEGER)
RETURNS TABLE(
    entity_type VARCHAR,
    entity_id INTEGER,
    operation VARCHAR,
    performer_name VARCHAR,
    performed_at TIMESTAMP
) AS $$
BEGIN
    RETURN QUERY
    SELECT 
        al.entity_type,
        al.entity_id,
        al.operation,
        COALESCE(u.name, 'system') as performer_name,
        al.performed_at
    FROM audit_log al
    LEFT JOIN users u ON al.performed_by = u.user_id
    WHERE al.performed_by = p_user_id
    ORDER BY al.performed_at DESC;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION trg_recalculate_order_price()
RETURNS TRIGGER AS $$
BEGIN
    UPDATE orders o
    SET total_price = (
        SELECT COALESCE(SUM(oi.quantity * p.price), 0)
        FROM order_items oi
        JOIN products p ON oi.product_id = p.product_id
        WHERE oi.order_id = o.order_id
    )
    WHERE o.order_id IN (
        SELECT DISTINCT order_id
        FROM order_items
        WHERE product_id = NEW.product_id
    )
    AND o.status = 'pending';
    
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION trg_audit_orders()
RETURNS TRIGGER AS $$
DECLARE
    v_user_id INTEGER;
BEGIN
    BEGIN
        v_user_id := current_setting('app.user_id')::INTEGER;
    EXCEPTION 
        WHEN OTHERS THEN
            RETURN COALESCE(NEW, OLD);
    END;

    IF v_user_id IS NULL THEN
        RETURN COALESCE(NEW, OLD);
    END IF;

    INSERT INTO audit_log(
        entity_type, 
        entity_id, 
        operation, 
        performed_by,
        performed_at
    ) VALUES (
        'order',
        COALESCE(NEW.order_id, OLD.order_id),
        TG_OP,
        v_user_id,
        CURRENT_TIMESTAMP
    );
    
    RETURN COALESCE(NEW, OLD);
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION trg_order_status_history()
RETURNS TRIGGER AS $$
DECLARE
    v_user_id INTEGER;
BEGIN
    BEGIN
        v_user_id := current_setting('app.user_id')::INTEGER;
    EXCEPTION 
        WHEN OTHERS THEN
            RETURN NEW;
    END;
    
    IF v_user_id IS NULL THEN
        RETURN NEW;
    END IF;
    
    IF OLD.status IS DISTINCT FROM NEW.status THEN
        INSERT INTO order_status_history(
            order_id, 
            old_status, 
            new_status, 
            changed_by,
            changed_at
        ) VALUES (
            NEW.order_id,
            OLD.status,
            NEW.status,
            v_user_id,
            CURRENT_TIMESTAMP
        );
    END IF;
    
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION trg_audit_products()
RETURNS TRIGGER AS $$
DECLARE
    v_user_id INTEGER;
BEGIN
    BEGIN
        v_user_id := current_setting('app.user_id')::INTEGER;
    EXCEPTION 
        WHEN OTHERS THEN
            RETURN COALESCE(NEW, OLD);
    END;

    IF v_user_id IS NULL THEN
        RETURN COALESCE(NEW, OLD);
    END IF;

    INSERT INTO audit_log(
        entity_type, 
        entity_id, 
        operation, 
        performed_by,
        performed_at
    ) VALUES (
        'product',
        COALESCE(NEW.product_id, OLD.product_id),
        TG_OP,
        v_user_id,
        CURRENT_TIMESTAMP
    );
    
    RETURN COALESCE(NEW, OLD);
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION trg_audit_users()
RETURNS TRIGGER AS $$
DECLARE
    v_user_id INTEGER;
BEGIN
    BEGIN
        v_user_id := current_setting('app.user_id')::INTEGER;
    EXCEPTION 
        WHEN OTHERS THEN
            RETURN COALESCE(NEW, OLD);
    END;
    
    IF v_user_id IS NULL THEN
        RETURN COALESCE(NEW, OLD);
    END IF;
    
    INSERT INTO audit_log(
        entity_type, 
        entity_id, 
        operation, 
        performed_by,
        performed_at
    ) VALUES (
        'user',
        COALESCE(NEW.user_id, OLD.user_id),
        TG_OP,
        v_user_id,
        CURRENT_TIMESTAMP
    );
    
    RETURN COALESCE(NEW, OLD);
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION generate_csv_report()
RETURNS TEXT AS $$
DECLARE
    csv_content TEXT;
BEGIN
    SELECT string_agg(
        o.order_id || ',' ||
        COALESCE(u.name, '') || ',' ||
        COALESCE(o.status, '') || ',' ||
        COALESCE(o.total_price::TEXT, '0') || ',' ||
        COALESCE(o.order_date::TEXT, '') || ',' ||
        COALESCE(osh.old_status, '') || ',' ||
        COALESCE(osh.new_status, '') || ',' ||
        COALESCE(osh.changed_at::TEXT, '') || ',' ||
        COALESCE(al.operation, '') || ',' ||
        COALESCE(u2.name, '') || ',' ||
        COALESCE(al.performed_at::TEXT, ''),
        E'\n'
        ORDER BY o.order_date DESC NULLS LAST
    ) INTO csv_content
    FROM orders o
    LEFT JOIN users u ON o.user_id = u.user_id
    LEFT JOIN (
        SELECT DISTINCT ON (order_id) order_id, old_status, new_status, changed_at
        FROM order_status_history
        ORDER BY order_id, changed_at DESC
    ) osh ON o.order_id = osh.order_id
    LEFT JOIN (
        SELECT DISTINCT ON (entity_id) entity_id, operation, performed_by, performed_at
        FROM audit_log
        WHERE entity_type = 'order'
        ORDER BY entity_id, performed_at DESC
    ) al ON o.order_id = al.entity_id
    LEFT JOIN users u2 ON al.performed_by = u2.user_id;
    
    RETURN 'order_id,user_name,status,total_price,order_date,old_status,new_status,changed_at,operation,performed_by_name,performed_at' || 
           E'\n' || 
           COALESCE(csv_content, '');
END;
$$ LANGUAGE plpgsql;